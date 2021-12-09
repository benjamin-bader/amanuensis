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

#pragma once

#include <cassert>
#include <cstdint>
#include <type_traits>

namespace ama { namespace trusty {

namespace Bytes {

template <typename T>
inline T from_network_order(const uint8_t* bytes)
{
    static_assert(std::is_integral<T>::value && sizeof(T) == 4, "T must be a four-byte-wide integer");

    assert(bytes != nullptr);

    return (static_cast<T>(bytes[0]) << 24)
         | (static_cast<T>(bytes[1]) << 16)
         | (static_cast<T>(bytes[2]) <<  8)
         | (static_cast<T>(bytes[3])      );
}

template <typename T>
inline void to_network_order(const T val, uint8_t* bytes)
{
    static_assert(std::is_integral<T>::value && sizeof(T) == 4, "T must be a four-byte-wide integer");

    assert(bytes != nullptr);

    bytes[0] = static_cast<uint8_t>((val >> 24) & 0xFF);
    bytes[1] = static_cast<uint8_t>((val >> 16) & 0xFF);
    bytes[2] = static_cast<uint8_t>((val >>  8) & 0xFF);
    bytes[3] = static_cast<uint8_t>((val      ) & 0xFF);
}

} // namespace Bytes

}} // ama::trusty
