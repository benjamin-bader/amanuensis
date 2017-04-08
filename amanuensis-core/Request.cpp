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

#include "Request.h"

Request::Request(const std::string &method,
                 const std::string &uri,
                 const int major,
                 const int minor,
                 const std::vector<Header> &headers,
                 const std::vector<uint8_t> &body) :
    method_(method),
    uri_(uri),
    major_version_(major),
    minor_version_(minor),
    headers_({headers}),
    body_(body)
{
    // nothing
}

std::string Request::method() const
{
    return method_;
}

std::string Request::uri() const
{
    return uri_;
}

int Request::major_version() const
{
    return major_version_;
}

int Request::minor_version() const
{
    return minor_version_;
}

const Headers Request::headers() const
{
    return headers_;
}

const std::vector<uint8_t> Request::body() const
{
    return body_;
}
