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

#include <cctype>
#include <sstream>

Request::Request() :
    method_(),
    uri_(),
    major_version_(0),
    minor_version_(0),
    headers_(),
    body_()
{

}

Request::Request(const std::string &method,
                 const std::string &uri,
                 const int major,
                 const int minor,
                 const Headers &headers,
                 const std::vector<uint8_t> &body) :
    method_(method),
    uri_(uri),
    major_version_(major),
    minor_version_(minor),
    headers_(headers),
    body_(body)
{
    // nothing
}

const std::string& Request::method() const
{
    return method_;
}

const std::string& Request::uri() const
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

const Headers& Request::headers() const
{
    return headers_;
}

const std::vector<uint8_t>& Request::body() const
{
    return body_;
}

const std::string Request::body_as_string() const
{
    return std::string(body_.begin(), body_.end());
}

const std::vector<uint8_t> Request::make_buffer() const
{
    std::stringstream ss;
    std::for_each(method_.begin(), method_.end(), [&ss](char c) { ss << std::toupper(c); });
    ss << " " << uri_ << " HTTP/" << major_version_ << "/" << minor_version_ << "\r\n";

    for (auto &header : headers_)
    {
        ss << header.name() << ": " << header.value() << "\r\n";
    }

    ss << "\r\n";

    auto str = ss.str();
    std::vector<uint8_t> result(str.begin(), str.end());

    return result;
}
