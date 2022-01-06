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

bool Request::can_persist() const
{
    if (message_.major_version() == 1 && message_.minor_version() == 0)
    {
        // RFC 7230 § 6.3: Persistence
        // A proxy server MUST NOT maintain a persistent connection with an
        // HTTP/1.0 client (see Section 19.7.1 of [RFC2068] for information and
        // discussion of the problems with the Keep-Alive header field
        // implemented by many HTTP/1.0 clients).
        return false;
    }

    auto connectionOpts = headers().find_by_name(QStringLiteral("Connection"));
    if (connectionOpts.contains(QStringLiteral("close")))
    {
        return false;
    }

    // We're HTTP 1.1 or greater and haven't been told to close the connection.
    return true;
}

}
