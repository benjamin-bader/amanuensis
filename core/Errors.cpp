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

#include "Errors.h"

using namespace ama;

namespace {

class ProxyErrorCategory : public std::error_category
{
public:
    virtual const char* name() const noexcept override;
    virtual std::string message(int ev) const noexcept override;
};

const char* ProxyErrorCategory::name() const noexcept
{
    return "ProxyError";
}

std::string ProxyErrorCategory::message(int ev) const noexcept
{
    switch (static_cast<ProxyError>(ev))
    {
    case ProxyError::NetworkError:
        return "network error";

    case ProxyError::RemoteDnsLookupError:
        return "failed to resolve remote hostname";

    case ProxyError::ClientDisconnected:
        return "client connection unexpectedly closed";

    case ProxyError::RemoteDisconnected:
        return "remote connection unexpectedly closed";

    case ProxyError::MalformedRequest:
        return "client HTTP request is malformed";

    case ProxyError::MalformedResponse:
        return "remote HTTP response is malformed";

    default:
        return "(unrecognized error)";
    }
}

const ProxyErrorCategory category = {};

} // namespace

std::error_code ama::make_error_code(ProxyError pe)
{
    return {static_cast<int>(pe), category};
}
