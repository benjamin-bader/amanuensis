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
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "TrustyCommon.h"

namespace ama { namespace trusty {

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
            throw std::runtime_error("Connected is unexpectedly closed");
        }

        data += num_read;
        len -= num_read;
    }
}

} // namespace

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
    uint8_t *ptr = reinterpret_cast<uint8_t *>(&n);
    payload.assign(ptr, ptr + sizeof(n));
}

void Message::assign_i32_payload(int n)
{
    uint8_t *ptr = reinterpret_cast<uint8_t *>(&n);
    payload.assign(ptr, ptr + sizeof(n));
}

void Message::assign_string_payload(const std::string &str)
{
    payload.assign(str.begin(), str.end());
}

int Message::get_i32_payload() const
{
    if (payload.size() != sizeof(int))
    {
        std::stringstream ss;
        ss << "Expected " << sizeof(int) << " bytes; got " << payload.size();
        throw std::invalid_argument(ss.str());
    }

    return *((int *) payload.data());
}

uint32_t Message::get_u32_payload() const
{
    if (payload.size() != sizeof(uint32_t))
    {
        std::stringstream ss;
        ss << "Expected " << sizeof(uint32_t) << " bytes; got " << payload.size();
        throw std::invalid_argument(ss.str());
    }

    return *((uint32_t *) payload.data());
}

std::string Message::get_string_payload() const
{
    return std::string(payload.begin(), payload.end());
}

/////////

const size_t MessageProcessor::kBufLen; // needs to be defined here, but its value is given in the header file.

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

    long opts = ::fcntl(socket_fd, F_GETFL, NULL);
    opts |= O_NONBLOCK;
    ::fcntl(socket_fd, F_SETFL, opts);

    int connect_result = ::connect(socket_fd, (const struct sockaddr *) &address, sizeof(address));
    if (connect_result == -1)
    {
        int error_value = errno;
        if (error_value != EINPROGRESS)
        {
            std::cerr << "Failed to connect; errno=" << error_value << std::endl;

            close(socket_fd);
            throw std::system_error(errno, std::system_category());
        }

        do
        {
            timeval tv { 0, 100000 }; // 1/10 of a second
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(socket_fd, &fds);

            int select_result = ::select(socket_fd + 1, NULL, &fds, NULL, &tv);
            if (select_result < 0 && errno != EINTR)
            {
                // problem
                close(socket_fd);
                throw std::system_error(errno, std::system_category());
            }

            if (select_result == 0)
            {
                // timeout
                close(socket_fd);
                throw ama::timeout_exception();
            }

            int valopt;
            socklen_t lon = sizeof(int);
            if (::getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0)
            {
                // Can't getsockopt
                close(socket_fd);
                throw std::system_error(errno, std::system_category());
            }

            if (valopt > 0)
            {
                // socket wasn't selected for write, ergo we aren't connected.
                // not a timeout though...
                close(socket_fd);
                throw std::runtime_error("connection failed");
            }

            // We're connected!
            break;
        } while(true);
    }

    // Clear O_NONBLOCK
    opts = ::fcntl(socket_fd, F_GETFL, NULL);
    opts &= (~O_NONBLOCK);
    ::fcntl(socket_fd, F_SETFL, opts);

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

        checked_read(fd, read_buffer, to_read);
        result.payload.insert(result.payload.end(), &read_buffer[0], &read_buffer[to_read]);

        len -= to_read;
    }

    return result;
}

}} // ama::trusty
