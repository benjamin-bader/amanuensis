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

#include <iostream>

#include <Security/Security.h>

#include "Service.h"
#include "TrustyCommon.h"

using namespace ama::trusty;

namespace
{

class AuthorizedService : public IService
{
public:
    AuthorizedService(IService *delegate, AuthorizationRef auth);
    virtual ~AuthorizedService();

    virtual const std::string get_http_proxy_host() override;
    virtual int get_http_proxy_port() override;

    virtual void set_http_proxy_host(const std::string &host) override;
    virtual void set_http_proxy_port(int port) override;

    virtual void reset_proxy_settings() override;

private:
    void assert_right(const char *right);

private:
    IService *delegate_;
    AuthorizationRef auth_;
};

AuthorizedService::AuthorizedService(IService *delegate, AuthorizationRef auth)
    : delegate_(delegate)
    , auth_(auth)
{

}

AuthorizedService::~AuthorizedService()
{
    AuthorizationFree(auth_, kAuthorizationFlagDefaults);
}

const std::string AuthorizedService::get_http_proxy_host()
{
    assert_right(ama::kGetHostRightName);
    return delegate_->get_http_proxy_host();
}

int AuthorizedService::get_http_proxy_port()
{
    assert_right(ama::kGetPortRightName);
    return delegate_->get_http_proxy_port();
}

void AuthorizedService::set_http_proxy_host(const std::string &host)
{
    assert_right(ama::kSetHostRightName);
    delegate_->set_http_proxy_host(host);
}

void AuthorizedService::set_http_proxy_port(int port)
{
    assert_right(ama::kSetPortRightName);
    delegate_->set_http_proxy_port(port);
}

void AuthorizedService::reset_proxy_settings()
{
    assert_right(ama::kClearSettingsRightName);
    delegate_->reset_proxy_settings();
}

void AuthorizedService::assert_right(const char *right)
{
    std::cerr << "Asserting a right: " << right << std::endl;
}

const Message kAck { MessageType::Ack, {} };

} // namespace

class ClientConnection::impl
{
public:
    impl(IService *service, int client_fd);

    void handle();

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

    if (hello.payload.size() != sizeof(AuthorizationExternalForm))
    {
        processor_.send({ MessageType::Error, {} });
        throw std::invalid_argument("Payload isn't the right size");
    }

    AuthorizationRef authRef;
    OSStatus status = AuthorizationCreateFromExternalForm((AuthorizationExternalForm *) hello.payload.data(), &authRef);
    if (status != errAuthorizationSuccess)
    {
        processor_.send({ MessageType::Error, {} });
        throw std::invalid_argument("payload couldn't be made into an AuthorizationRef");
    }

    processor_.send(kAck);

    service_ = std::make_unique<AuthorizedService>(service, authRef);
}

void ClientConnection::impl::handle()
{
    Message msg;
    do
    {
        msg = processor_.recv();

        switch (msg.type)
        {
        case MessageType::SetProxyHost:
        {
            std::string requested_host(msg.payload.begin(), msg.payload.end());
            service_->set_http_proxy_host(requested_host);
            processor_.send(kAck);
            break;
        }

        case MessageType::SetProxyPort:
        {
            int port = *((int*) msg.payload.data());
            service_->set_http_proxy_port(port);
            processor_.send(kAck);
            break;
        }

        case MessageType::GetProxyHost:
        {
            auto host = service_->get_http_proxy_host();

            Message reply;
            reply.type = MessageType::Ack;
            reply.payload.assign(host.begin(), host.end());

            processor_.send(reply);
            break;
        }

        case MessageType::GetProxyPort:
        {
            auto port = service_->get_http_proxy_port();

            uint8_t *result_bytes = (uint8_t *) &port;

            Message reply;
            reply.type = MessageType::Ack;
            reply.payload.assign(result_bytes, result_bytes + sizeof(port));

            processor_.send(reply);
            break;
        }

        case MessageType::ClearProxySettings:
        {
            service_->reset_proxy_settings();
            processor_.send(kAck);
            break;
        }

        case MessageType::Disconnect:
        {
            // Done!
            break;
        }

        default:
        {
            std::cerr << "ERROR: Received response message-type from a client!  type=" << msg.type << std::endl;
            break;
        }

        }; // switch
    } while (msg.type != MessageType::Disconnect);
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
