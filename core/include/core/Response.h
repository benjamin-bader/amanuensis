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

#include "core/common.h"
#include "core/global.h"

#include "core/Headers.h"
#include "core/HttpMessage.h"

#include <QString>

namespace ama
{

class HttpMessageParser;

class A_EXPORT Response
{
public:
    Response();
    Response(const Response&) = default;
    Response(Response&&) = default;
    virtual ~Response() = default;

    Response& operator=(const Response&) = default;
    Response& operator=(Response&&) = default;

    int major_version() const noexcept { return message_.major_version(); }
    int minor_version() const noexcept { return message_.major_version(); }

    Headers& headers() { return message_.headers(); }
    const Headers& headers() const { return message_.headers(); }

    int status_code() const { return message_.status_code(); }
    const QString status_message() const { return message_.status_message(); }

    QByteArray body() { return message_.body(); }

    friend class HttpMessageParser;

private:
    HttpMessage message_;
};

} // namespace ama
