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

#ifndef REQUEST_H
#define REQUEST_H

#include <cstdint>
#include <string>
#include <map>
#include <tuple>
#include <vector>

#include "Headers.h"

#include "global.h"

class A_EXPORT Request
{
public:
    typedef std::vector<Header>::const_iterator const_iterator;

    Request(const std::string &method,
            const std::string &uri,
            const int major,
            const int minor,
            const std::vector<Header> &headers,
            const std::vector<uint8_t> &body);

    std::string method() const;
    std::string uri() const;

    int major_version() const;
    int minor_version() const;

    const Headers headers() const;

    const std::vector<uint8_t> body() const;

private:
    std::string method_;
    std::string uri_;
    int major_version_;
    int minor_version_;

    Headers headers_;

    std::vector<uint8_t> body_;
};

#endif // REQUEST_H
