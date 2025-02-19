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

#include "XpcServer.h"

#include "constants.h"

#include "log/Log.h"
#include "trusty/CFLog.h"
#include "trusty/CFRef.h"
#include "trusty/IMessage.h"

#include <Availability.h>
#include <CoreFoundation/CoreFoundation.h>
#include <security/Security.h>

#include <bsm/libbsm.h> // for audit_token_t

#include <functional>
#include <string>
#include <type_traits>

extern "C" void xpc_connection_get_audit_token(xpc_connection_t, audit_token_t*);

namespace ama::trusty {

template <> class is_cfref<SecCodeRef>              : public std::true_type {};
template <> class is_cfref<SecTaskRef>              : public std::true_type {};
template <> class is_cfref<SecRequirementRef>       : public std::true_type {};

namespace {

bool validate_xpc_message_or_connection(xpc_object_t message_or_connection)
{
    if (__builtin_available(macOS 12, *))
    {
        // This is handled natively by XPC
        log::info("validate_xpc_message_or_connection: XPC handles this automatically per-message");
        return true;
    }

    CFRef<SecCodeRef> codeRef = nullptr;

    SecCodeRef rawCodeRef = nullptr;
    OSStatus os;

    if (__builtin_available(macOS 11, *))
    {
        os = SecCodeCreateWithXPCMessage(message_or_connection, kSecCSDefaultFlags, &rawCodeRef);
        if (os != errSecSuccess)
        {
            CFRef<CFStringRef> errorMessage = SecCopyErrorMessageString(os, NULL);
            log::error("SecCodeCreateWithXPCMessage failed", log::CFStringValue("error_message", errorMessage));
            return false;
        }
    }
    else
    {
        audit_token_t token;
        xpc_connection_get_audit_token((xpc_connection_t) message_or_connection, &token);

        CFRef<SecTaskRef> taskRef = SecTaskCreateWithAuditToken(kCFAllocatorDefault, token);
        CFRef<CFDictionaryRef> dict = CFDictionaryCreate(kCFAllocatorDefault, (const void**)&kSecGuestAttributePid, (const void**)&token, 1, NULL, NULL);

        os = SecCodeCopyGuestWithAttributes(NULL, dict, kSecCSDefaultFlags, &rawCodeRef);
        if (os != errSecSuccess)
        {
            CFRef<CFStringRef> errorMessage = SecCopyErrorMessageString(os, NULL);
            log::error("SecCodeCopyGuestWithAttributes failed", log::CFStringValue("error_message", errorMessage));
            return false;
        }
    }

    codeRef = rawCodeRef;
    rawCodeRef = nullptr;

    SecRequirementRef rawRequirementRef = NULL;
    os = SecRequirementCreateWithString(CFSTR(CODE_SIGNING_REQUIREMENT), kSecCSDefaultFlags, &rawRequirementRef);
    if (os != errSecSuccess)
    {
        CFRef<CFStringRef> errorMessage = SecCopyErrorMessageString(os, NULL);
        log::error("SecRequirementCreateWithString failed", log::CFStringValue("error_message", errorMessage));
        return false;
    }

    CFRef<SecRequirementRef> requirementRef = rawRequirementRef;
    rawRequirementRef = nullptr;

    CFErrorRef errors = NULL;
    os = SecCodeCheckValidityWithErrors(codeRef, kSecCSDefaultFlags, requirementRef, &errors);
    if (os != errSecSuccess)
    {
        CFRef<CFStringRef> description = CFErrorCopyDescription(errors);
        CFRelease(errors);

        log::error("Code signature did NOT match designated requirements",
                    log::CStrValue("requirements", CODE_SIGNING_REQUIREMENT),
                    log::CFStringValue("error", description));

        return false;
    }

    return true;
}

} // namespace

struct ConnectionRecord
{
    std::weak_ptr<XpcServer> server;
    std::uint64_t connectionId;
};

// This is an xpc_finalizer_t.  It will be invoked by the XPC runtime when a remote connection
// is being cleaned up - i.e., non-deterministically.  Its job is to a) delete the heap-allocated
// ConnectionRecord pointer, and b) attempt to clean up per-connection state in the parent
// XpcServer.
void CleanUpConnection(void* context)
{
    if (context == nullptr)
    {
        return;
    }

    std::unique_ptr<ConnectionRecord> record(reinterpret_cast<ConnectionRecord*>(context));

    if (auto server = record->server.lock())
    {
        server->m_connections.erase(record->connectionId);
    }
}

XpcServer::XpcServer(const std::shared_ptr<IService>& service, const char* machServiceName)
    : m_service{service}
    , m_machServiceName{machServiceName}
    , m_listener{NULL}
    , m_nextId{1}
    , m_connections{}
{
}

XpcServer::~XpcServer()
{    
    m_connections.clear();

    if (m_listener != NULL)
    {
        xpc_connection_cancel(m_listener);
        xpc_release(m_listener);
        m_listener = NULL;
    }
}

void XpcServer::serve()
{
    log::info("Creating match service listener...");

    m_listener = xpc_connection_create_mach_service(m_machServiceName, dispatch_get_main_queue(), XPC_CONNECTION_MACH_SERVICE_LISTENER);
    if (m_listener == NULL)
    {
        log::log_event(log::Severity::Fatal, "Failed to open Mach service", log::CStrValue("service_name", m_machServiceName));
        return;
    }

    auto weakSelf = weak_from_this();
    xpc_connection_set_event_handler(m_listener, ^(xpc_object_t connection) {
                                         if (auto self = weakSelf.lock())
                                         {
                                             self->handleIncomingConnection((xpc_connection_t) connection);
                                         }
                                         else
                                         {
                                             xpc_connection_cancel((xpc_connection_t) connection);
                                         }
                                     });

    log::info("Activating listener...");
    xpc_connection_activate(m_listener);

    log::info("Server active!");
    dispatch_main();
}

void XpcServer::handleIncomingConnection(xpc_connection_t connection)
{
    log::info("Incoming connection, checking signature...");
    if (! isClientSignatureValid(connection))
    {
        log::info("Client signature is NOT VALID, dropping the connection");
        return;
    }

    log::info("Client signature is valid, establishing connection");

    ConnectionRecord* context = new ConnectionRecord;
    context->server = weak_from_this();
    context->connectionId = m_nextId++;

    xpc_connection_set_context(connection, context);
    xpc_connection_set_finalizer_f(connection, CleanUpConnection);

    auto weakSelf = weak_from_this();
    xpc_connection_set_event_handler(connection, ^(xpc_object_t message) {
                                         if (auto self = weakSelf.lock())
                                         {
                                             self->handleIncomingMessage(message);
                                         }
                                         else
                                         {
                                             xpc_connection_cancel(xpc_dictionary_get_remote_connection(message));
                                         }
                                     });

    xpc_connection_activate(connection);
}

bool XpcServer::isClientSignatureValid(xpc_connection_t connection)
{
    if (__builtin_available(macOS 12, *))
    {
        log::info("Setting peer codesigning requirement", log::CStrValue("requirement", CODE_SIGNING_REQUIREMENT));
        return xpc_connection_set_peer_code_signing_requirement(connection, CODE_SIGNING_REQUIREMENT) != 0;
    }

    if (__builtin_available(macOS 11, *))
    {
        // We'll be manually validating per-message
        log::info("Manually validating per-message");
        return true;
    }

    log::info("Falling back to validate_xpc_message_or_connection");
    return validate_xpc_message_or_connection((xpc_object_t) connection);
}

void XpcServer::handleIncomingMessage(xpc_object_t message)
{
    xpc_connection_t remoteConnection = xpc_dictionary_get_remote_connection(message);
    xpc_type_t messageType = xpc_get_type(message);
    if (messageType == XPC_TYPE_ERROR)
    {
        const char* description = xpc_dictionary_get_string(message, XPC_ERROR_KEY_DESCRIPTION);

        log::error("XPC error event received for remote peer", log::CStrValue("desc", description));

        if (__builtin_available(macOS 12, *))
        {
            if (message == XPC_ERROR_PEER_CODE_SIGNING_REQUIREMENT)
            {
                // womp womp
                log::error("XPC says - signature invalid");
                xpc_connection_cancel(remoteConnection);
                return;
            }
        }

        if (message == XPC_ERROR_CONNECTION_INVALID)
        {
            // Client hung up - nbd.
            return;
        }

        if (message == XPC_ERROR_TERMINATION_IMMINENT)
        {
            // Client is _going_ to hang up - nbd.
            return;
        }

        // no clue
        log::error("I don't know what this error means.");
        return;
    }

    if (! isMessageSignatureValid(message))
    {
        log::error("failed to validate code signature on inbound message");
        xpc_connection_cancel(remoteConnection);
        return;
    }

    auto clientMessage = Protocol::unwrap(message);
    if (clientMessage == nullptr)
    {
        log::error("Incomprehensible client message");
        return;
    }

    auto connectionId = getConnectionId(remoteConnection);
    XRef<xpc_object_t> reply = xpc_dictionary_create_reply(message);
    std::unique_ptr<IMessage> replyMessage;
    try
    {
        auto svc = m_connections.find(connectionId);
        if (svc != m_connections.end())
        {
            replyMessage = handleOne(svc->second, std::move(clientMessage));
        }
        else if (clientMessage->type() == MessageType::Hello)
        {
            // This is a new connection!
            std::vector<std::uint8_t> authExternalForm;
            authExternalForm.resize(xpc_data_get_length(clientMessage->payload()));
            const void* pData =  xpc_data_get_bytes_ptr(clientMessage->payload());
            memcpy(authExternalForm.data(), pData, authExternalForm.size());

            m_connections.emplace(connectionId, std::make_shared<AuthorizedService>(m_service, authExternalForm));

            replyMessage = Protocol::create_message(MessageType::Hello, xpc_null_create());
        }
        else
        {
            xpc_object_t desc = xpc_string_create("Internal error - connection not found");
            replyMessage = Protocol::create_message(MessageType::Error, desc);
            xpc_release(desc);
        }
    }
    catch (const std::exception& ex)
    {
        xpc_object_t desc = xpc_string_create(ex.what());
        replyMessage = Protocol::create_message(MessageType::Error, desc);
        xpc_release(desc);
    }

    Protocol::wrap(reply, replyMessage);
    xpc_connection_send_message(remoteConnection, reply);
}

std::unique_ptr<IMessage> XpcServer::handleOne(const std::shared_ptr<AuthorizedService>& svc, std::unique_ptr<IMessage>&& message)
{
    switch (message->type())
    {
    case MessageType::Hello:
    {
        throw std::invalid_argument{"Hello message should have been handled elsewhere"};
    }

    case MessageType::GetToolVersion:
    {
        auto toolVersion = svc->get_current_version();
        XRef<xpc_object_t> versionObj = xpc_uint64_create(toolVersion);
        return Protocol::create_message(message->type(), versionObj);
    }

    case MessageType::GetProxyState:
    {
        auto proxyState = svc->get_http_proxy_state();
        XRef<xpc_object_t> stateObj = proxyState.to_xpc();
        return Protocol::create_message(message->type(), stateObj);
    }

    case MessageType::SetProxyState:
    {
        ProxyState updatedSettings{message->payload()};
        svc->set_http_proxy_state(updatedSettings);
        return Protocol::create_message(message->type(), xpc_null_create());
    }

    case MessageType::ClearProxySettings:
    {
        svc->reset_proxy_settings();
        return Protocol::create_message(message->type(), xpc_null_create());
    }

    default:
    {
        std::string description("Unexpected MessageType: ");
        description += static_cast<std::int64_t>(message->type());
        XRef<xpc_object_t> errorObj = xpc_string_create(description.c_str());
        return Protocol::create_message(MessageType::Error, errorObj);
    }

    }
}

bool XpcServer::isMessageSignatureValid(xpc_object_t message)
{
    if (__builtin_available(macOS 11, *))
    {
        return validate_xpc_message_or_connection(message);
    }
    return true;
}

std::uint64_t XpcServer::getConnectionId(xpc_connection_t connection) const
{
    void* context = xpc_connection_get_context(connection);
    if (context == nullptr)
    {
        log::error("remote connection has no context");
        throw std::invalid_argument{"remote connection has no context"};
    }

    return reinterpret_cast<ConnectionRecord*>(context)->connectionId;
}

} // ama::trusty
