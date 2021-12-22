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

#include <xpc/xpc.h>

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

    ProxyState(xpc_object_t dict);

    ProxyState(const ProxyState&) = default;
    ProxyState(ProxyState&&) = default;
    ProxyState& operator=(const ProxyState&) = default;
    ProxyState& operator=(ProxyState&&) = default;

    bool is_enabled() const noexcept { return enabled_; }
    const std::string& get_host() const noexcept { return host_; }
    int32_t get_port() const noexcept { return port_; }

    /**
     * @brief to_xpc
     * @return a *retained* XPC dictionary representation of this ProxyState.
     */
    xpc_object_t to_xpc() const;

private:
    bool enabled_;
    std::string host_;
    int32_t port_;
};

}} // namespace ama::trusty
