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

#include <cstdint>
#include <vector>
#include <string>

namespace ama {
namespace trusty {

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
     * Tells the recipient to set the system's proxy host to the
     * hostname specified in the payload.
     *
     * The payload is a length-prefixed string containing the proxy
     * hostname.
     */
    SetProxyHost = 2,

    /**
     * Tells the recipient to set the system's proxy port to the number
     * specified in the payload.
     *
     * The payload is a uint16_t.
     */
    SetProxyPort = 3,

    /**
     *
     */
    GetProxyHost = 4,

    /**
     *
     */
    GetProxyPort = 5,

    /**
     * Clears all custom proxy settings, restoring system defaults.
     */
    ClearProxySettings = 6,

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
};

class MessageProcessor
{
public:
    MessageProcessor(const std::string &path);
    MessageProcessor(int fd);
    ~MessageProcessor();

    void send(const Message &message) const;
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
    static const size_t kBufLen = 64;

    /**
     * A file-descriptor of a connected socket.
     */
    int fd;

    /**
     * A buffer used when reading data from [fd].
     */
    uint8_t read_buffer[kBufLen];
};

} // namespace trusty
} // namespace ama

#endif // COMMAND_H
