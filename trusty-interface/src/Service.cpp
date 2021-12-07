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

#include "trusty/Service.h"

#include <algorithm>
#include <utility>

#include "trusty/MessageProcessor.h"

namespace ama { namespace trusty {

namespace
{

class MessageProcessorServiceClient : public IService
{
public:
    MessageProcessorServiceClient(const std::string &address);
    ~MessageProcessorServiceClient() override;

    ProxyState get_http_proxy_state() override;
    virtual void set_http_proxy_state(const ProxyState& endpoint) override;

    void reset_proxy_settings() override;

    uint32_t get_current_version() override;

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

ProxyState MessageProcessorServiceClient::get_http_proxy_state()
{
    Message msg { MessageType::GetProxyState, {} };

    processor.send(msg);

    msg = processor.recv();
    if (msg.type != MessageType::Ack)
    {
        throw std::runtime_error{msg.get_string_payload()};
    }

    return ProxyState{msg.payload};
}

void MessageProcessorServiceClient::set_http_proxy_state(const ProxyState &endpoint)
{
    Message msg { MessageType::SetProxyState, endpoint.serialize() };

    processor.send(msg);

    msg = processor.recv();
    if (msg.type != MessageType::Ack)
    {
        throw std::runtime_error{msg.get_string_payload()};
    }
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

uint32_t MessageProcessorServiceClient::get_current_version()
{
    processor.send({ MessageType::GetToolVersion, {} });

    Message ack = processor.recv();
    if (ack.type != MessageType::Ack)
    {
        throw std::invalid_argument("Unexpected response from helper tool");
    }

    return ack.get_u32_payload();
}

void MessageProcessorServiceClient::authenticate(const std::vector<uint8_t> &auth)
{
    processor.send({ MessageType::Hello, auth });

    Message ack = processor.recv();
    if (ack.type != MessageType::Ack)
    {
        // auth failed!
        throw std::invalid_argument("Authorization failed");
    }
}

} // namespace

std::unique_ptr<IService> create_client(const std::string &address, const std::vector<uint8_t> &auth)
{
    std::unique_ptr<MessageProcessorServiceClient> svc = std::make_unique<MessageProcessorServiceClient>(address);
    svc->authenticate(auth);

    return std::move(svc);
}

}} // namespace ama::trusty
