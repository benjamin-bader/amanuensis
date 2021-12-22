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

#include "trusty/ProxyState.h"

namespace ama { namespace trusty {

namespace {

constexpr size_t kHeaderSize = 5;

constexpr const char kFieldEnabled[] = "enabled";
constexpr const char kFieldHost[] = "host";
constexpr const char kFieldPort[] = "port";

} // namepace

ProxyState::ProxyState(bool enabled, const std::string &host, int port) noexcept
    : enabled_(enabled)
    , host_(host)
    , port_(port)
{}

ProxyState::ProxyState(xpc_object_t dict)
{
    xpc_object_t maybeEnabled = xpc_dictionary_get_value(dict, kFieldEnabled);
    xpc_object_t maybeHost = xpc_dictionary_get_value(dict, kFieldHost);
    xpc_object_t maybePort = xpc_dictionary_get_value(dict, kFieldPort);

    if (maybeEnabled == nullptr || maybeHost == nullptr || maybePort == nullptr)
    {
        throw std::invalid_argument{"missing one of enabled, host, and/or port"};
    }

    if (xpc_get_type(maybeEnabled) != XPC_TYPE_BOOL || xpc_get_type(maybeHost) != XPC_TYPE_STRING || xpc_get_type(maybePort) != XPC_TYPE_INT64)
    {
        throw std::invalid_argument{"unexpected type for one of enabled, host, or port"};
    }

    enabled_ = xpc_bool_get_value(maybeEnabled);
    host_ = xpc_string_get_string_ptr(maybeHost);
    port_ = static_cast<int>(xpc_int64_get_value(maybePort));
}

xpc_object_t ProxyState::to_xpc() const
{
    auto result = xpc_dictionary_create_empty();
    xpc_dictionary_set_bool(result, kFieldEnabled, enabled_);
    xpc_dictionary_set_string(result, kFieldHost, host_.c_str());
    xpc_dictionary_set_int64(result, kFieldPort, static_cast<int64_t>(port_));
    return result;
}

}} // namespace ama::trusty
