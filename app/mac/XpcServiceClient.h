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

#include "trusty/CFRef.h"
#include "trusty/IMessage.h"
#include "trusty/IService.h"

#include <xpc/xpc.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace ama {

class XpcServiceClient : public trusty::IService, public std::enable_shared_from_this<XpcServiceClient>
{
public:
    XpcServiceClient(const std::string& serviceName, const std::vector<std::uint8_t>& authBytes);
    virtual ~XpcServiceClient() noexcept;

    void activate();

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
    trusty::ProxyState get_http_proxy_state() override;

    /**
     * Updates the HTTP proxy settings for the currently-active
     * network service.
     *
     * If the given state has an empty hostname, service implementations
     * will also set the proxy to disabled.
     *
     * @param state
     */
    void set_http_proxy_state(const trusty::ProxyState& state) override;

    /**
     * Undo any settings alterations made by Trusty, restoring the system's
     * proxy settings to their initial state.
     *
     * Not implemented, and may possibly be removed in favor of implementing
     * state in Amanuensis itself, instead of in Trusty.
     */
    void reset_proxy_settings() override;

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
    uint32_t get_current_version() override;

private:
    void on_unexpected_xpc_event(xpc_object_t event);

    /**
     * @brief Sends a message to the remote Trusty XPC service.
     * @param type the type of the message being sent.
     * @param payload
     * The message payload.  Note that THIS METHOD TAKES OWNERSHIP OF THE PAYLOAD.
     * The payload will be released before this method ends!
     *
     * @return the response payload.
     */
    trusty::XRef<xpc_object_t> send_message(trusty::MessageType type, xpc_object_t payload);

private:
    std::mutex mux_;
    xpc_connection_t conn_;
};

}
