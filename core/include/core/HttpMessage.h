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

#include "core/global.h"
#include "core/Headers.h"

#include <QByteArray>
#include <QString>

namespace ama
{

class A_EXPORT HttpMessage
{
public:
    friend class HttpMessageParser;

    HttpMessage();
    HttpMessage(const HttpMessage&) = default;
    HttpMessage(HttpMessage&&) = default;

    HttpMessage& operator=(const HttpMessage&) = default;
    HttpMessage& operator=(HttpMessage&&) = default;

    const QString method() const;
    const QString uri() const;

    int status_code() const;
    const QString status_message() const;

    int major_version() const;
    int minor_version() const;

    Headers& headers();
    const Headers& headers() const;

    void set_method(const QString& method);
    void set_uri(const QString& uri);
    void set_major_version(int major_version);
    void set_minor_version(int minor_version);
    void set_body(const QByteArray& body);
    void set_body(QByteArray&& body);

    void set_status_code(int status_code);
    void set_status_message(const QString& message);

    void add_header(const QString& name, const QString& value);

    // Return the body as a string, using any specified Content-Encoding
    // if present.
    const QString body_as_string() const;

    QByteArray body();
    const QByteArray body() const;
private:
    // Request-specific data
    QString method_;
    QString uri_;

    // Response-specific data
    int status_code_;
    QString status_message_;

    int major_version_;
    int minor_version_;

    Headers headers_;

    QByteArray body_;
};

} // namespace ama
