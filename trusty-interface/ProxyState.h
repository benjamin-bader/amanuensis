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

#ifndef PROXYSTATE_H
#define PROXYSTATE_H

#include <cstdint>
#include <string>
#include <vector>

namespace ama { namespace trusty {

/**
 * Plain Old Data representing HTTP proxy configuration
 * data on macOS.
 *
 * Specifically, ProxyState is a 3-tuple of (enabled, host, port);
 * this is true for all of the network protocols that we care
 * about.
 */
class ProxyState
{
public:
    ProxyState(bool enabled, const std::string &host, int port) noexcept;

    /**
     * Initializes ProxyState with the serialized data in the given
     * payload, which is expected to have been produced from
     * ProxyState::serialize().
     *
     * @param payload a serialized binary representation of ProxyState data.
     * @throws if the given payload is not comprehensible as a ProxyState.
     */
    ProxyState(const std::vector<uint8_t>& payload);

    ProxyState(const ProxyState&) = default;
    ProxyState(ProxyState&&) = default;
    ProxyState& operator=(const ProxyState&) = default;
    ProxyState& operator=(ProxyState&&) = default;

    bool is_enabled() const noexcept { return enabled_; }
    const std::string& get_host() const noexcept { return host_; }
    int32_t get_port() const noexcept { return port_; }

    /**
     * Returns a representation of this instance as a vector of bytes,
     * suitable for transmitting over the wire.
     *
     * @return a vector of bytes representing the data in this instance.
     */
    std::vector<uint8_t> serialize() const;

private:
    bool enabled_;
    std::string host_;
    int32_t port_;
};

}} // namespace ama::trusty

#endif // PROXYSTATE_H
