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

#ifndef UNIXSOCKET_H
#define UNIXSOCKET_H

#include <cstdint>
#include <string>

namespace ama { namespace trusty {

class ISocket
{
public:
    virtual ~ISocket() = default;

    /**
     * Fully writes the given data, throwing if all the data cannot be written.
     *
     * @param data a pointer to the data to be written
     * @param len the number of bytes to be written
     * @throws std::system_error on failure.
     */
    virtual void checked_write(uint8_t* data, size_t len) = 0;

    /**
     * Fully reads the given amount of data, throwing if the expected amount
     * of data cannot be read.
     *
     * @param data a pointer to the data buffer.
     * @param len the number of bytes to read.
     * @throws std::system_error on failure.
     */
    virtual void checked_read(uint8_t* data, size_t len) = 0;
};

/**
 * Implements ISocket using a UNIX-family socket, initialized
 * either with an already-open file descriptor or with the absolute
 * filesystem path of the socket to be opened.
 *
 * Closes the file descriptor on destruction.
 */
class UnixSocket : public ISocket
{
public:
    /**
     * Initializes a UnixSocket by opening the socket at the given path.
     * @param path the path of the socket to open.
     */
    UnixSocket(const std::string& path);

    /**
     * Initializes a UnixSocket, taking ownership of the given (open) file descriptor.
     * @param fd a descriptor for an open socket.
     */
    UnixSocket(int fd) noexcept;

    ~UnixSocket() noexcept;

    void checked_write(uint8_t* data, size_t len) override;

    void checked_read(uint8_t* data, size_t len) override;

private:
    int fd_;
};

}} // namespace ama::trusty


#endif // UNIXSOCKET_H
