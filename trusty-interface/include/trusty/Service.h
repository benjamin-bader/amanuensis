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

#ifndef ISERVICE_H
#define ISERVICE_H

#include <cstdint>
#include <memory>

#include "trusty/ProxyState.h"

namespace ama { namespace trusty {

/**
 * Defines the RPC interface between Amanuensis and Trusty.
 *
 * IService is a pure, virtual, synchronous, interface.  Errors
 * are communicated via exceptions - all interface methods are
 * permitted to throw, and callers are advised to expect exceptions.
 */
class IService
{
public:
    virtual ~IService() {}

    /**
     * Gets the HTTP proxy settings for the currently-active
     * network service.
     *
     * If no settings are configured, the return will be
     * default-initialized as disabled, with an empty hostname.
     * There is no distinction between having an empty hostname
     * configured in the system preferences and having _no_ hostname
     * so configured; therefore, that distinction is not modeled.
     *
     * @return a ProxyState instance containing current HTTP proxy settings.
     */
    virtual ProxyState get_http_proxy_state() = 0;

    /**
     * Updates the HTTP proxy settings for the currently-active
     * network service.
     *
     * If the given state has an empty hostname, service implementations
     * will also set the proxy to disabled.
     *
     * @param state
     */
    virtual void set_http_proxy_state(const ProxyState& state) = 0;

    /**
     * Undo any settings alterations made by Trusty, restoring the system's
     * proxy settings to their initial state.
     *
     * Not implemented, and may possibly be removed in favor of implementing
     * state in Amanuensis itself, instead of in Trusty.
     */
    virtual void reset_proxy_settings() = 0;

    /**
     * Gets a number identifying the current service version.
     *
     * Version changes are introduced whenever the trusty-interface
     * or trusty code changes.  They are used to indicate that a new
     * version of the priviliged helper tool needs to be installed.
     *
     * The version number is defined in the file "TrustyCommon.h".
     *
     * @return the current version number.
     */
    virtual uint32_t get_current_version() = 0;
};

/**
 * Creates an IService implementation, and connects it to
 * the server listening on the given UNIX socket @p path.
 *
 * @param path an absolute path to a UNIX socket.
 * @return a unique pointer to the connected IService.
 * @throws throws an exception if creating or connecting fails.
 */
std::unique_ptr<IService> create_client(const std::string &path, const std::vector<uint8_t> &auth);

}} // namespace ama::trusty


#endif // ISERVICE_H
