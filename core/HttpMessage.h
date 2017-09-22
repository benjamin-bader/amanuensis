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

#ifndef HTTPMESSAGE_H
#define HTTPMESSAGE_H

#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#include "Headers.h"

#include "global.h"

namespace ama
{

class A_EXPORT HttpMessage
{
public:
    friend class HttpMessageParser;

    HttpMessage();

    const std::string& method() const;
    const std::string& uri() const;

    int status_code() const;
    const std::string& status_message() const;

    int major_version() const;
    int minor_version() const;

    Headers& headers();
    const Headers& headers() const;

    void set_method(const std::string &method);
    void set_uri(const std::string &uri);
    void set_major_version(int major_version);
    void set_minor_version(int minor_version);
    void set_body(const std::vector<uint8_t> &body);

    void set_status_code(int status_code);
    void set_status_message(const std::string &message);

    void add_header(const std::string &name, const std::string &value);

    // Return the body as a string, using any specified Content-Encoding
    // if present.
    const std::string body_as_string() const;

    std::vector<uint8_t>& body();
    const std::vector<uint8_t>& body() const;
private:
    // Request-specific data
    std::string method_;
    std::string uri_;

    // Response-specific data
    int status_code_;
    std::string status_message_;

    int major_version_;
    int minor_version_;

    Headers headers_;

    std::vector<uint8_t> body_;
};

} // namespace ama

#endif // HTTPMESSAGE_H
