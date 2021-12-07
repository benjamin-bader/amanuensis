// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2021 Benjamin Bader
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

#pragma once

#include <cstdint>
#include <cstdlib>

namespace ama::trusty {

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
    virtual void checked_write(const uint8_t* data, size_t len) = 0;

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

}
