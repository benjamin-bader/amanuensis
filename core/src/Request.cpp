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

#include <QDataStream>
#include <QTextStream>

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

const QByteArray Request::format() const noexcept
{
    QString result;
    QTextStream ds(&result);

    ds << method() << " " << uri() << " HTTP/" << message_.major_version() << "." << message_.minor_version() << "\r\n";

    auto hds = headers();
    for (const auto& name : hds.names())
    {
        bool first = true;
        ds << name << ": ";

        for (const auto& value : hds.find_by_name(name))
        {
            if (!first)
            {
                ds << ", ";
            }
            ds << value;
            first = false;
        }

        ds << "\r\n";
    }
    ds << "\r\n";

    if (body().size() > 0)
    {
        ds << body();
    }

    return result.toLatin1();
}

}
