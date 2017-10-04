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

#include "Service.h"

#include "MessageProcessor.h"

using namespace ama::trusty;

namespace
{

class MessageProcessorServiceClient : public IService
{
public:
    MessageProcessorServiceClient(const std::string &address);
    virtual ~MessageProcessorServiceClient() override;

    virtual void set_http_proxy_host(const std::string &host) override;
    virtual void set_http_proxy_port(int port) override;

    virtual const std::string get_http_proxy_host() override;
    virtual int get_http_proxy_port() override;

    virtual void reset_proxy_settings() override;

public:
    void authenticate(const std::vector<uint8_t> &auth);

private:
    MessageProcessor processor;
};

MessageProcessorServiceClient::MessageProcessorServiceClient(const std::string &path)
    : IService()
    , processor(path)
{
    // no-op
}

MessageProcessorServiceClient::~MessageProcessorServiceClient()
{
    Message msg { MessageType::Disconnect, {} };
    try
    {
        processor.send(msg);
    }
    catch (...)
    {
        // best-effort
    }
}

void MessageProcessorServiceClient::set_http_proxy_host(const std::string &host)
{
    Message msg;
    msg.type = MessageType::SetProxyHost;
    msg.payload.assign(host.begin(), host.end());

    processor.send(msg);

    msg = processor.recv();
    if (msg.type != MessageType::Ack)
    {
        std::string error_message(msg.payload.begin(), msg.payload.end());
        throw std::invalid_argument(error_message);
    }
}

void MessageProcessorServiceClient::set_http_proxy_port(int port)
{
    uint8_t *port_bytes = (uint8_t *) &port;

    Message msg;
    msg.type = MessageType::SetProxyPort;
    msg.payload.assign(port_bytes, port_bytes + sizeof(int));

    processor.send(msg);

    msg = processor.recv();
    if (msg.type != MessageType::Ack)
    {
        std::string error_message(msg.payload.begin(), msg.payload.end());
        throw std::invalid_argument(error_message);
    }
}

const std::string MessageProcessorServiceClient::get_http_proxy_host()
{
    Message msg;
    msg.type = MessageType::GetProxyHost;
    msg.payload.clear();

    processor.send(msg);

    msg = processor.recv();
    if (msg.type != MessageType::Ack)
    {
        std::string error_message(msg.payload.begin(), msg.payload.end());
        throw std::invalid_argument(error_message);
    }

    return std::string(msg.payload.begin(), msg.payload.end());
}

int MessageProcessorServiceClient::get_http_proxy_port()
{
    Message msg;
    msg.type = MessageType::GetProxyPort;
    msg.payload.clear();

    processor.send(msg);

    msg = processor.recv();
    if (msg.type != MessageType::Ack)
    {
        std::string error_message(msg.payload.begin(), msg.payload.end());
        throw std::invalid_argument(error_message);
    }

    if (msg.payload.size() != sizeof(int))
    {
        throw std::invalid_argument("Expected a sizeof(int) payload");
    }

    return *((int*) msg.payload.data());
}

void MessageProcessorServiceClient::reset_proxy_settings()
{
    Message msg;
    msg.type = MessageType::ClearProxySettings;
    msg.payload.clear();

    processor.send(msg);

    msg = processor.recv();
    if (msg.type != MessageType::Ack)
    {
        std::string error_message(msg.payload.begin(), msg.payload.end());
        throw std::invalid_argument(error_message);
    }
}

void MessageProcessorServiceClient::authenticate(const std::vector<uint8_t> &auth)
{
    Message hello = { MessageType::Hello, auth };
    processor.send(hello);

    Message ack = processor.recv();
    if (ack.type != MessageType::Ack)
    {
        // auth failed!
        throw std::invalid_argument("Authorization failed");
    }
}

}

std::unique_ptr<IService> ama::trusty::create_client(const std::string &address, const std::vector<uint8_t> &auth)
{
    std::unique_ptr<MessageProcessorServiceClient> svc = std::make_unique<MessageProcessorServiceClient>(address);
    svc->authenticate(auth);

    return std::move(svc);
}
