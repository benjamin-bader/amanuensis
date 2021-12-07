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

#include "core/Request.h"

#include <sstream>
#include <utility>

namespace ama {

Request::Request(const HttpMessage& message)
    : message_(message)
{
}

Request::Request(HttpMessage &&message)
    : message_(std::move(message))
{
}

const std::string Request::format() const noexcept
{
    std::stringstream ss;
    ss << method() << " " << this->uri() << " HTTP/" << this->message_.major_version() << "." << message_.minor_version() << "\r\n";

    auto hds = headers();
    for (auto i = 0; i < hds.size(); ++i)
    {
        const auto& name = hds.names()[i];
        const auto& value = hds.values()[i];

        ss << name << ": " << value << "\r\n";
    }
    ss << "\r\n";

    if (body().size() > 0)
    {
        std::for_each(body().begin(), body().end(), [&](auto byte) {
            ss << static_cast<char>(byte);
        });
    }

    return ss.str();
}

}
