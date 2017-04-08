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

#ifndef RESPONSE_H
#define RESPONSE_H

#include <cstdint>
#include <string>
#include <vector>

#include "Headers.h"

#include "global.h"

class A_EXPORT Response
{
public:
    Response(int status, const std::string &message, const Headers &headers, const std::vector<uint8_t> &body);

    const int status_code() const;
    const std::string message() const;

    const Headers headers() const;

    const std::vector<uint8_t> body() const;

private:
    int status_code_;
    std::string message_;

    Headers headers_;

    std::vector<uint8_t> body_;
};

#endif // RESPONSE_H
