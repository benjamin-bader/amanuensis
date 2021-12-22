// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2021 Benjamin Bader
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "mac/XpcServiceClient.h"

#include "trusty/CFLog.h"
#include "trusty/CFRef.h"
#include "trusty/IMessage.h"

#include <system_error>

namespace ama {

XpcServiceClient::XpcServiceClient(const std::string& serviceName, const std::vector<std::uint8_t>& authBytes)
{
    log::info("Opening XPC connection", log::StringValue("mach_service_name", serviceName));
    conn_ = xpc_connection_create_mach_service(serviceName.c_str(), NULL, XPC_CONNECTION_MACH_SERVICE_PRIVILEGED);

    log::info("Connection opened?");
    if (conn_ == nullptr)
    {
        throw std::domain_error{"failed to connect to trusty"};
    }

    auto weakSelf = weak_from_this();
    xpc_connection_set_event_handler(conn_, ^(xpc_object_t event)
                                     {
                                         if (auto self = weakSelf.lock())
                                         {
                                             self->on_unexpected_xpc_event(event);
                                         }
                                     });

    xpc_connection_activate(conn_);

    log::info("Authorizing...");
    xpc_object_t authPayload = xpc_data_create(authBytes.data(), authBytes.size());
    send_message(trusty::MessageType::Hello, authPayload);
    log::info("Authorized!");
}

XpcServiceClient::~XpcServiceClient() noexcept
{
    try
    {
        std::lock_guard guard{mux_};

        if (conn_ != nullptr)
        {
            xpc_connection_cancel(conn_);
            xpc_release(conn_);
            conn_ = nullptr;
        }
    }
    catch (...)
    {
        // wat do
    }
}

void XpcServiceClient::activate()
{
    auto weakSelf = weak_from_this();
    xpc_connection_set_event_handler(conn_, ^(xpc_object_t event)
                                     {
                                         if (auto self = weakSelf.lock())
                                         {
                                             self->on_unexpected_xpc_event(event);
                                         }
                                     });
    xpc_connection_activate(conn_);
}

trusty::ProxyState XpcServiceClient::get_http_proxy_state()
{
    auto reply = send_message(trusty::MessageType::GetProxyState, xpc_null_create());

    return trusty::ProxyState{reply};
}

void XpcServiceClient::set_http_proxy_state(const trusty::ProxyState &state)
{
    auto reply = send_message(trusty::MessageType::SetProxyState, state.to_xpc());

    if (xpc_get_type(reply) == XPC_TYPE_NULL)
    {
        return;
    }

    throw std::invalid_argument{"unexpected error"};
}

void XpcServiceClient::reset_proxy_settings()
{
    auto reply = send_message(trusty::MessageType::ClearProxySettings, xpc_null_create());

    if (xpc_get_type(reply) == XPC_TYPE_NULL)
    {
        return;
    }

    throw std::invalid_argument{"unexpected error"};
}

uint32_t XpcServiceClient::get_current_version()
{
    auto reply = send_message(trusty::MessageType::GetToolVersion, xpc_null_create());

    if (xpc_get_type(reply) != XPC_TYPE_UINT64)
    {
        throw std::invalid_argument{"unexpected error"};
    }

    return static_cast<uint32_t>(xpc_uint64_get_value(reply));
}

trusty::XRef<xpc_object_t> XpcServiceClient::send_message(trusty::MessageType type, xpc_object_t payload)
{
    trusty::XRef<xpc_object_t> releasingPayload{payload};

    auto msg = trusty::Protocol::create_message(type, payload);
    trusty::XRef<xpc_object_t> wrapper = xpc_dictionary_create_empty();
    trusty::Protocol::wrap(wrapper, msg);

    trusty::XRef<xpc_object_t> replyWrapper = xpc_connection_send_message_with_reply_sync(conn_, wrapper);

    if (xpc_get_type(replyWrapper) == XPC_TYPE_ERROR)
    {
        // wat do
        const char* error = xpc_dictionary_get_string(replyWrapper, XPC_ERROR_KEY_DESCRIPTION);
        throw std::runtime_error{error};
    }

    auto reply = trusty::Protocol::unwrap(replyWrapper);
    if (reply == nullptr)
    {
        // good lord
        throw std::invalid_argument{"wat"};
    }

    if (reply->type() == trusty::MessageType::Error)
    {
        const char* error = xpc_string_get_string_ptr(reply->payload());
        throw std::runtime_error{error};
    }

    if (reply->type() != type)
    {
        // wtf
        throw std::invalid_argument{"nope"};
    }

    return trusty::XRef<xpc_object_t>{xpc_retain(reply->payload())};
}

void XpcServiceClient::on_unexpected_xpc_event(xpc_object_t event)
{
    auto type = xpc_get_type(event);
    if (type == XPC_TYPE_ERROR)
    {
        const char* desc = xpc_dictionary_get_string(event, XPC_ERROR_KEY_DESCRIPTION);
        log::error("Whoops!", log::CStrValue("desc", desc));

        std::lock_guard guard(mux_);
        if (conn_ != nullptr)
        {
            xpc_connection_cancel(conn_);
            xpc_release(conn_);
            conn_ = nullptr;
        }
        return;
    }
    else
    {
        log::error("Got an unanticipated message!");
    }
}

}
