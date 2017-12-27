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

#include "UnixSocket.h"

#include <errno.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>
#include <stdexcept>
#include <system_error>

#include "TrustyCommon.h"

namespace ama { namespace trusty {

UnixSocket::UnixSocket(const std::string &path)
    : fd_(0)
{
    int socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        int error_code = errno;
        std::cerr << "socket() failed: " << error_code << std::endl;
        throw std::system_error(error_code, std::system_category());
    }

    struct sockaddr_un address = {};
    ::memset(&address, 0, sizeof(address));

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

            ::close(socket_fd);
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
                ::close(socket_fd);
                throw std::system_error(errno, std::system_category());
            }

            if (select_result == 0)
            {
                // timeout
                ::close(socket_fd);
                throw ama::timeout_exception();
            }

            int valopt;
            socklen_t lon = sizeof(int);
            if (::getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0)
            {
                // Can't getsockopt
                ::close(socket_fd);
                throw std::system_error(errno, std::system_category());
            }

            if (valopt > 0)
            {
                // socket wasn't selected for write, ergo we aren't connected.
                // not a timeout though...
                ::close(socket_fd);
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

    this->fd_ = socket_fd;
}

UnixSocket::UnixSocket(int fd) noexcept
    : fd_(fd)
{
}

UnixSocket::~UnixSocket() noexcept
{
    if (fd_ != 0)
    {
        ::close(fd_);
    }
}

void UnixSocket::checked_write(uint8_t* data, size_t len)
{
    while (len > 0)
    {
        ssize_t num_written = ::write(fd_, data, len);
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

void UnixSocket::checked_read(uint8_t* data, size_t len)
{
    while (len > 0)
    {
        ssize_t num_read = ::read(fd_, data, len);
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
            throw std::runtime_error("Connection is unexpectedly closed");
        }

        data += num_read;
        len -= num_read;
    }
}

}} // namespace ama::trusty
