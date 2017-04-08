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

#include "Response.h"

Response::Response(int status, const std::string &message, const Headers &headers, const std::vector<uint8_t> &body) :
    status_code_(status),
    message_(message),
    headers_(headers),
    body_(body)
{

}

const int Response::status_code() const
{
    return status_code_;
}

const std::string Response::message() const
{
    return message_;
}

const Headers Response::headers() const
{
    return headers_;
}

const std::vector<uint8_t> Response::body() const
{
    return body_;
}
