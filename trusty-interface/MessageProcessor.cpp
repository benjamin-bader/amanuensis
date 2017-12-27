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

#include "MessageProcessor.h"

#include <iostream>
#include <sstream>
#include <string>

#include "Bytes.h"
#include "TrustyCommon.h"

namespace ama { namespace trusty {

std::ostream& operator<<(std::ostream &out, const MessageType &type)
{
    switch (type)
    {
    case MessageType::Hello:              return out << "MessageType::Hello";
    case MessageType::Ack:                return out << "MessageType::Ack";
    case MessageType::Error:              return out << "MessageType::Error";
    case MessageType::SetProxyState:      return out << "MessageType::SetProxyState";
    case MessageType::GetProxyState:      return out << "MessageType::GetProxyState";
    case MessageType::ClearProxySettings: return out << "MessageType::ClearProxySettings";
    case MessageType::GetToolVersion:     return out << "MessageType::GetToolVersion";
    case MessageType::Disconnect:         return out << "MessageType::Disconnect";
    }

    std::stringstream ss;
    ss << "Unrecognized message type: " << static_cast<uint8_t>(type);
    throw std::invalid_argument(ss.str());
}

/////////

void Message::assign_u8_payload(uint8_t n)
{
    payload = { n };
}

void Message::assign_u32_payload(uint32_t n)
{
    payload.resize(sizeof(uint32_t));
    Bytes::to_network_order(n, payload.data());
}

void Message::assign_i32_payload(int32_t n)
{
    payload.resize(sizeof(int32_t));
    Bytes::to_network_order(n, payload.data());
}

void Message::assign_string_payload(const std::string &str)
{
    payload.assign(str.begin(), str.end());
}

int Message::get_i32_payload() const
{
    if (payload.size() != sizeof(int32_t))
    {
        std::stringstream ss;
        ss << "Expected " << sizeof(int) << " bytes; got " << payload.size();
        throw std::invalid_argument(ss.str());
    }

    return Bytes::from_network_order<int32_t>(payload.data());
}

uint32_t Message::get_u32_payload() const
{
    if (payload.size() != sizeof(uint32_t))
    {
        std::stringstream ss;
        ss << "Expected " << sizeof(uint32_t) << " bytes; got " << payload.size();
        throw std::invalid_argument(ss.str());
    }

    return Bytes::from_network_order<uint32_t>(payload.data());
}

std::string Message::get_string_payload() const
{
    return std::string(payload.begin(), payload.end());
}

/////////

constexpr size_t MessageProcessor::kBufLen; // needs to be defined here, but its value is given in the header file.

MessageProcessor::MessageProcessor(const std::string &path)
    : socket_(std::make_unique<UnixSocket>(path))
{
}

MessageProcessor::MessageProcessor(int fd)
    : socket_(std::make_unique<UnixSocket>(fd))
{
}

MessageProcessor::MessageProcessor(std::unique_ptr<ISocket>&& socket)
    : socket_(std::move(socket))
{
}

MessageProcessor::~MessageProcessor()
{
}

/* The wire format is simple.  A message is written as:
 * type: 1 octet
 * payload length: 4 octets, network order (i.e. big-endian)
 * payload: <payload length> octets
 */

void MessageProcessor::send(const Message &message)
{
    uint8_t type = static_cast<uint8_t>(message.type);
    uint32_t payload_length = static_cast<uint32_t>(message.payload.size());

    socket_->checked_write(&type, sizeof(uint8_t));
    socket_->checked_write((uint8_t *) &payload_length, sizeof(uint32_t));
    socket_->checked_write((uint8_t *) message.payload.data(), message.payload.size());
}

Message MessageProcessor::recv()
{
    Message result;
    uint8_t type;
    uint32_t length;

    socket_->checked_read(&type, sizeof(uint8_t));
    socket_->checked_read((uint8_t *) &length, sizeof(uint32_t));

    // I think there's a theoretical bug here, in that size_t isn't
    // *guaranteed* to be 32 bits wide; it could, technically, be only
    // 16 bits.  In this case, a payload larger than 0xFFFF would yield
    // undefined behavior.
    //
    // We can, as a practical matter, ignore this for now, as we control
    // both sender and receiver in all cases.  Also, size_t is at least
    // 32 bits wide on all platforms I know about that run macOS.

    static_assert(sizeof(size_t) >= sizeof(uint32_t), "size_t should be convertible to uint32_t");

    result.type = static_cast<MessageType>(type);
    result.payload.reserve(static_cast<size_t>(length));

    size_t len = static_cast<size_t>(length);
    while (len > 0)
    {
        size_t to_read = std::min(len, kBufLen);

        socket_->checked_read(read_buffer, to_read);
        result.payload.insert(result.payload.end(), &read_buffer[0], &read_buffer[to_read]);

        len -= to_read;
    }

    return result;
}

}} // ama::trusty
