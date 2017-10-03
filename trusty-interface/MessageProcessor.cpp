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
#include <system_error>

#include <errno.h>
#include <poll.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

using namespace ama::trusty;

namespace
{

/**
 * @brief Fully writes the given data to a file descriptor,
 *        throwing if all the data cannot be written.
 *
 * @param fd an open socket descriptor
 * @param data a pointer to the data to be written
 * @param len the number of bytes to be written
 * @throws std::system_error on failure.
 */
template <typename T, size_t S = sizeof(T)>
void checked_write(int fd, T* data, size_t len)
{
    static_assert(S == 1, "Only byte-sized types enabled");

    while (len > 0)
    {
        ssize_t num_written = ::write(fd, data, len);
        if (num_written == -1)
        {
            int ec = errno;
            if (ec == EINTR)
            {
                // try again
                continue;
            }

            throw std::system_error(ec, std::system_category());
        }

        data += num_written;
        len -= num_written;
    }
}

template <typename T, size_t S = sizeof(T)>
void checked_read(int fd, T* data, size_t len)
{
    static_assert(S == 1, "Only byte-sized types are allowed");

    while (len > 0)
    {
        ssize_t num_read = ::read(fd, data, len);
        if (num_read == -1)
        {
            int ec = errno;
            if (ec == EINTR)
            {
                continue;
            }

            throw std::system_error(ec, std::system_category());
        }

        if (num_read == 0)
        {
            throw std::domain_error("Connected is unexpectedly closed");
        }

        data += num_read;
        len -= num_read;
    }
}

} // namespace

std::ostream& ama::trusty::operator<<(std::ostream &out, const MessageType &type)
{
    switch (type)
    {
    case MessageType::Ack:                return out << "MessageType::Ack";
    case MessageType::Error:              return out << "MessageType::Error";
    case MessageType::SetProxyHost:       return out << "MessageType::SetProxyHost";
    case MessageType::SetProxyPort:       return out << "MessageType::SetProxyPort";
    case MessageType::GetProxyHost:       return out << "MessageType::GetProxyHost";
    case MessageType::GetProxyPort:       return out << "MessageType::GetProxyPort";
    case MessageType::ClearProxySettings: return out << "MessageType::ClearProxySettings";
    case MessageType::Disconnect:         return out << "MessageType::Disconnect";
    }

    std::stringstream ss;
    ss << "Unrecognized message type: " << static_cast<uint8_t>(type);
    throw std::invalid_argument(ss.str());
}

const size_t MessageProcessor::kBufLen;

MessageProcessor::MessageProcessor(const std::string &path)
    : fd(0)
{
    int socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        int error_code = errno;
        std::cerr << "socket() failed: " << error_code << std::endl;
        throw std::system_error(error_code, std::system_category());
    }

    struct sockaddr_un address = {};
    ::bzero(&address, sizeof(address));

    address.sun_family = AF_UNIX;
    ::strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path) - 1);

    int connect_result = ::connect(socket_fd, (const struct sockaddr *) &address, sizeof(address));
    if (connect_result == -1)
    {
        int error_value = errno;
        std::cerr << "Failed to connect; errno=" << error_value << std::endl;

        close(socket_fd);
        throw std::system_error(error_value, std::system_category());
    }

    this->fd = socket_fd;
}

MessageProcessor::MessageProcessor(int fd)
    : fd(fd)
{
    // no-op
}

MessageProcessor::~MessageProcessor()
{
    if (fd != 0)
    {
        close(fd);
    }
}

/* The wire format is simple.  A message is written as:
 * type: 1 octet
 * payload length: 4 octets
 * payload: <payload length> octets
 */

void MessageProcessor::send(const Message &message) const
{
    uint8_t type = static_cast<uint8_t>(message.type);
    uint32_t payload_length = static_cast<uint32_t>(message.payload.size());

    checked_write(fd, &type, sizeof(uint8_t));
    checked_write(fd, (uint8_t *) &payload_length, sizeof(uint32_t));
    checked_write(fd, (uint8_t *) message.payload.data(), message.payload.size());
}

Message MessageProcessor::recv()
{
    Message result;
    uint8_t type;
    uint32_t length;

    checked_read<uint8_t>(fd, &type, sizeof(uint8_t));
    checked_read<uint8_t>(fd, (uint8_t *) &length, sizeof(uint32_t));

    result.type = static_cast<MessageType>(type);
    result.payload.reserve(static_cast<size_t>(length));

    // I think there's a theoretical bug here, in that size_t isn't
    // *guaranteed* to be 32 bits wide; it could, technically, be only
    // 16 bits.  In this case, a payload larger than 0xFFFF would yield
    // undefined behavior.
    //
    // We can, as a practical matter, ignore this for now, as we control
    // both sender and receiver in all cases.
    size_t len = static_cast<size_t>(length);
    while (len > 0)
    {
        size_t to_read = std::min(len, kBufLen);

        checked_read(fd, read_buffer, to_read);
        result.payload.insert(result.payload.end(), &read_buffer[0], &read_buffer[to_read]);

        len -= to_read;
    }

    return result;
}
