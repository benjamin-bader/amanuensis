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

#include "core/HttpMessage.h"

#include <utility>

using namespace ama;

HttpMessage::HttpMessage() :
    method_(),
    uri_(),
    status_code_(0),
    status_message_(),
    major_version_(0),
    minor_version_(0),
    headers_(),
    body_()
{

}

const QString HttpMessage::method() const
{
    return method_;
}

const QString HttpMessage::uri() const
{
    return uri_;
}

int HttpMessage::status_code() const
{
    return status_code_;
}

const QString HttpMessage::status_message() const
{
    return status_message_;
}

int HttpMessage::major_version() const noexcept
{
    return major_version_;
}

int HttpMessage::minor_version() const noexcept
{
    return minor_version_;
}

Headers& HttpMessage::headers()
{
    return headers_;
}

const Headers& HttpMessage::headers() const
{
    return headers_;
}

const QByteArray HttpMessage::body() const
{
    return body_;
}

QByteArray HttpMessage::body()
{
    return body_;
}

void HttpMessage::set_method(const QString& method)
{
    method_ = method;
}

void HttpMessage::set_uri(const QString& uri)
{
    uri_ = uri;
}

void HttpMessage::set_major_version(int major_version)
{
    major_version_ = major_version;
}

void HttpMessage::set_minor_version(int minor_version)
{
    minor_version_ = minor_version;
}

void HttpMessage::set_body(const QByteArray& body)
{
    body_ = body;
}

void HttpMessage::set_body(QByteArray&& body)
{
    body_ = std::move(body);
}

void HttpMessage::set_status_code(int status_code)
{
    status_code_ = status_code;
}

void HttpMessage::set_status_message(const QString& message)
{
    status_message_ = message;
}

void HttpMessage::add_header(const QString& name, const QString& value)
{
    headers_.insert(name, value);;
}

const QString HttpMessage::body_as_string() const
{
    return QString::fromLocal8Bit(body_);
}
