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

#include <algorithm>
#include <iterator>
#include <utility>

#include "core/common.h"
#include "core/global.h"

#include "core/Headers.h"
#include "core/HttpMessage.h"

namespace ama
{

class HttpMessageParser;

class A_EXPORT Request
{
public:
    Request() = default;
    Request(const HttpMessage& message);
    Request(HttpMessage&& message);

    Request(const Request&) = default;
    Request(Request&&) = default;

    Request& operator=(const Request&) = default;
    Request& operator=(Request&&) = default;

    void set_major_version(int version) { message_.set_major_version(version); }
    void set_minor_version(int version) { message_.set_minor_version(version); }

    const std::string& method() const { return message_.method(); }
    void set_method(const std::string& method) { message_.set_method(method); }

    const std::string& uri() const { return message_.uri(); }
    void set_uri(const std::string& uri) { message_.set_uri(uri); }

    Headers& headers() { return message_.headers(); }
    const Headers& headers() const { return message_.headers(); }

    std::vector<uint8_t>& body() { return message_.body(); }
    const std::vector<uint8_t>& body() const { return message_.body(); }

    void set_body(const std::vector<uint8_t>& body) { message_.set_body(body); }
    void set_body(std::vector<uint8_t>&& body) { message_.set_body(std::move(body)); }
    void set_body(const std::string& body)
    {
        std::vector<uint8_t> bytes;
        bytes.reserve(body.size());
        std::copy(body.begin(), body.end(), std::back_inserter(bytes));
        message_.set_body(std::move(bytes));
    }

    const std::string format() const noexcept;

    friend class HttpMessageParser;

private:
    HttpMessage message_;
};

} // namespace ama

#endif // REQUEST_H
