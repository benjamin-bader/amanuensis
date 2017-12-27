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

#ifndef COMMAND_H
#define COMMAND_H

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "UnixSocket.h"

namespace ama { namespace trusty {

enum class MessageType : uint8_t
{
    /**
     * A reply, indicating that the last message was processed successfully.
     *
     * The payload is always empty.
     */
    Ack = 0,

    /**
     * A reply, indicating that the last message could not be processed.
     *
     * The payload is a length-prefixed string indicating the error.
     */
    Error = 1,

    /**
     * The first message sent by a connected client; establishes client
     * authorization.
     *
     * The payload is 32 bytes long, consisting of an AuthorizationExternalForm
     * struct.
     */
    Hello = 2,

    /**
     * Tells the recipient to set the system's proxy host to the
     * hostname specified in the payload.
     *
     * The payload is a serialized ama::trusty::ProxyState object.
     */
    GetProxyState = 3,

    /**
     * Tells the recipient to set the system's proxy port to the number
     * specified in the payload.
     *
     * The payload is a serialized ama::trusty::ProxyState object.
     */
    SetProxyState = 4,

    /**
     * Clears all custom proxy settings, restoring system defaults.
     */
    ClearProxySettings = 5,

    /**
     * Asks the receipient for its current version.
     *
     * The payload is empty.  A successful response will
     * be an 'Ack' message with a four-byte platform-endian
     * uint32_t version code.
     */
    GetToolVersion = 6,

    /**
     * Signals that the sender will terminate the connection, and the
     * receiver should close its, too.
     *
     * There is no payload.
     */
    Disconnect = UINT8_MAX,
};

std::ostream& operator<<(std::ostream &out, const MessageType &type);

struct Message
{
    MessageType type;
    std::vector<uint8_t> payload;

    Message() {}
    Message(MessageType type, const std::vector<uint8_t> &payload)
        : type(type)
        , payload(payload)
    {
        // no-op
    }

    Message(Message &&other)
        : type()
        , payload()
    {
        *this = std::move(other);
    }

    Message& operator=(Message&& other)
    {
        if (this != &other)
        {
            type = other.type;
            payload = std::move(other.payload);
        }
        return *this;
    }

    void assign_u8_payload(uint8_t n);
    void assign_u32_payload(uint32_t n);
    void assign_i32_payload(int32_t n);
    void assign_string_payload(const std::string &str);

    int get_i32_payload() const;
    uint32_t get_u32_payload() const;
    std::string get_string_payload() const;
};

class MessageProcessor
{
public:
    MessageProcessor(const std::string &path);
    MessageProcessor(int fd);
    MessageProcessor(std::unique_ptr<ISocket>&& socket);
    ~MessageProcessor();

    void send(const Message &message);
    Message recv();

private:
    MessageProcessor() = delete;
    MessageProcessor(const MessageProcessor&) = delete;
    MessageProcessor(MessageProcessor&&) = delete;

private:
    /**
     * The size, in bytes, of a MessageProcessor's [read_buffer].
     *
     * We can assume that nearly all payloads will be will be tiny;
     * 64 bytes should be more than enough.
     */
    static constexpr size_t kBufLen = 64;

    /**
     * A buffer used when reading data from [fd].
     */
    uint8_t read_buffer[kBufLen];

    /**
     * A file-descriptor of a connected socket.
     */
    std::unique_ptr<ISocket> socket_;
};

}} // ama::trusty

#endif // COMMAND_H
