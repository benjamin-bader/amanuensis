// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2017 Benjamin Bader
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

#include "ClientConnection.h"

#include <Security/Security.h>

#include <os/log.h>
#include <syslog.h>

#include "MessageProcessor.h"
#include "Service.h"
#include "TLog.h"
#include "TrustyCommon.h"

namespace ama { namespace trusty {

namespace {

class AuthorizedService : public IService
{
public:
    AuthorizedService(IService *delegate, const std::vector<uint8_t>& authExternalForm);
    ~AuthorizedService() override;

    ProxyState get_http_proxy_state() override;
    virtual void set_http_proxy_state(const ProxyState& endpoint) override;

    void reset_proxy_settings() override;

    uint32_t get_current_version() override;

private:
    void assert_right(const char *right);

private:
    IService *delegate_;
    AuthorizationRef auth_;
};

AuthorizedService::AuthorizedService(IService *delegate, const std::vector<uint8_t>& authExternalForm)
    : IService()
    , delegate_(delegate)
{
    if (authExternalForm.size() != sizeof(AuthorizationExternalForm))
    {
        throw std::invalid_argument("Payload isn't the right size");
    }

    OSStatus status = AuthorizationCreateFromExternalForm((AuthorizationExternalForm *) authExternalForm.data(), &auth_);
    if (status != errAuthorizationSuccess || auth_ == nullptr)
    {
        throw std::invalid_argument("payload couldn't be made into an AuthorizationRef");
    }
}

AuthorizedService::~AuthorizedService()
{
    if (auth_ != nullptr)
    {
        AuthorizationFree(auth_, kAuthorizationFlagDefaults);
    }
}

ProxyState AuthorizedService::get_http_proxy_state()
{
    assert_right(ama::kGetProxyStateRightName);
    return delegate_->get_http_proxy_state();
}

void AuthorizedService::set_http_proxy_state(const ProxyState &endpoint)
{
    assert_right(ama::kSetProxyStateRightName);
    delegate_->set_http_proxy_state(endpoint);
}

void AuthorizedService::reset_proxy_settings()
{
    assert_right(ama::kClearSettingsRightName);
    delegate_->reset_proxy_settings();
}

uint32_t AuthorizedService::get_current_version()
{
    assert_right(ama::kGetToolVersionRightName);
    return delegate_->get_current_version();
}

void AuthorizedService::assert_right(const char *right)
{
    log_debug("Asserting a right: {}", right);
    AuthorizationItem item = { right, 0, NULL, 0 };
    AuthorizationRights rights = { 1, &item };

    OSStatus status = AuthorizationCopyRights(
                auth_,
                &rights,
                kAuthorizationEmptyEnvironment,
                kAuthorizationFlagDefaults | kAuthorizationFlagExtendRights,
                nullptr);

    if (status != errAuthorizationSuccess)
    {
        log_debug("Could not obtain right non-interactively; trying again, with interaction (status={})", status);

        status = AuthorizationCopyRights(
                    auth_,
                    &rights,
                    kAuthorizationEmptyEnvironment,
                    kAuthorizationFlagDefaults | kAuthorizationFlagExtendRights | kAuthorizationFlagInteractionAllowed,
                    nullptr);

        if (status != errAuthorizationSuccess)
        {
            log_error("Right not present; status={}", status);
            throw std::invalid_argument("Authorization denied");
        }
    }
}

const Message kAck { MessageType::Ack, {} };

} // namespace

class ClientConnection::impl
{
public:
    impl(IService *service, int client_fd);

    void handle();
    MessageType handle_one();

private:
    std::unique_ptr<AuthorizedService> service_;
    MessageProcessor processor_;

};

ClientConnection::impl::impl(IService *service, int client_fd)
    : service_(nullptr)
    , processor_(client_fd)
{
    Message hello = processor_.recv();
    if (hello.type != MessageType::Hello)
    {
        processor_.send({ MessageType::Error, {} });
        throw std::invalid_argument("Didn't get a Hello message");
    }

    try
    {
        service_ = std::make_unique<AuthorizedService>(service, hello.payload);
    }
    catch (...)
    {
        processor_.send({ MessageType::Error, {} });
        throw;
    }

    processor_.send(kAck);
}

void ClientConnection::impl::handle()
{
    MessageType clientMessageType;
    do
    {
        try
        {
            clientMessageType = handle_one();
        }
        catch (const std::exception& ex)
        {
            Message err;
            err.type = MessageType::Error;
            err.assign_string_payload(std::string{ex.what()});

            try
            {
                processor_.send(err);
            }
            catch (const std::exception& ex2)
            {
                // don't crash while reporting an error
                log_critical("Error while sending error reply: %s", ex2.what());
            }

            break;
        }
    }
    while (clientMessageType != MessageType::Disconnect);
}

MessageType ClientConnection::impl::handle_one()
{
    Message msg = processor_.recv();

    switch (msg.type)
    {
    case MessageType::SetProxyState:
    {
        ProxyState state{msg.payload};
        service_->set_http_proxy_state(state);
        processor_.send(kAck);
        break;
    }

    case MessageType::GetProxyState:
    {
        auto state = service_->get_http_proxy_state();

        processor_.send({ MessageType::Ack, state.serialize() });
        break;
    }

    case MessageType::ClearProxySettings:
    {
        service_->reset_proxy_settings();
        processor_.send(kAck);
        break;
    }

    case MessageType::GetToolVersion:
    {
        uint32_t version = service_->get_current_version();

        Message reply;
        reply.type = MessageType::Ack;
        reply.assign_u32_payload(version);

        processor_.send(reply);
        break;
    }

    case MessageType::Disconnect:
    {
        // Done!
        break;
    }

    default:
    {
        log_critical("ERROR: Received response message-type from a client!  type={}", msg.type);
        break;
    }

    }; // switch

    return msg.type;
}

////////////

ClientConnection::ClientConnection(IService *service, int client_fd)
    : impl_(std::make_unique<impl>(service, client_fd))
{
}

ClientConnection::~ClientConnection()
{
    // no-op
}

void ClientConnection::handle()
{
    impl_->handle();
}

}} // ama::trusty
