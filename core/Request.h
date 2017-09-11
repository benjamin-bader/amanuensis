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

#pragma once

#include "common.h"
#include "global.h"

#include "Headers.h"
#include "HttpMessage.h"

namespace ama
{

class A_EXPORT Request
{
public:
    Request();

    const std::string& method() const { return message_.method(); }
    const std::string& uri() const { return message_.uri(); }

    Headers& headers() { return message_.headers(); }
    const Headers& headers() const { return message_.headers(); }

    HttpMessage& message() { return message_; }

private:
    HttpMessage message_;
};

} // namespace ama

#endif // REQUEST_H
