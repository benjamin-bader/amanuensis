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

#include "ProxyState.h"

#include "Bytes.h"

namespace ama { namespace trusty {

namespace {

constexpr size_t kHeaderSize = 5;

} // namepace

ProxyState::ProxyState(bool enabled, const std::string &host, int port) noexcept
    : enabled_(enabled)
    , host_(host)
    , port_(port)
{}

ProxyState::ProxyState(const std::vector<uint8_t> &payload)
{
    // Serialization format is:
    // 0: enabled (0 == disabled, !0 == enabled)
    // 1-4: port, as int32_t
    // 5-: host, as character data, not null-terminated.

    if (payload.size() < kHeaderSize)
    {
        throw std::invalid_argument{"payload too small to be a ProxyState"};
    }

    enabled_ = payload[0] != 0;
    port_ = Bytes::from_network_order<int32_t>(payload.data() + 1);

    if (payload.size() > kHeaderSize)
    {
        // This would be safe if payload.size() == kHeaderSize, but just to be paranoid,
        // let's avoid doing anything with a pointer to invalid memory.
        host_.assign(reinterpret_cast<const char *>(payload.data() + kHeaderSize), payload.size() - kHeaderSize);
    }
}

std::vector<uint8_t> ProxyState::serialize() const
{
    std::vector<uint8_t> payload(kHeaderSize);

    payload[0] = enabled_ ? 1 : 0;

    Bytes::to_network_order(port_, payload.data() + 1);

    std::copy(host_.begin(), host_.end(), std::back_inserter(payload));

    return payload;
}

}} // namespace ama::trusty
