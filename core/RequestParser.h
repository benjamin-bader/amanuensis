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

// Copyright (C)

#ifndef REQUESTPARSER_H
#define REQUESTPARSER_H

#include <cstdint>
#include <string>
#include <system_error>
#include <tuple>
#include <vector>

class QDebug;

#include "global.h"

#include "Headers.h"

class Request;

class A_EXPORT RequestParser
{
public:
    RequestParser();

    enum State {
        Incomplete = 0,
        Valid,
        Invalid
    };

    void reset();

    template <typename InputIterator>
    State parse(Request &request, InputIterator &begin, InputIterator end)
    {
        while (begin != end)
        {
            auto state = consume(request, *begin++);
            if (state != State::Incomplete)
            {
                return state;
            }
        }

        return Incomplete;
    }

private:
    friend QDebug operator<<(QDebug, const RequestParser &parser);

    State consume(Request &request, char input);

    enum ParserState {
        method_start,
        method,
        uri,
        http_version_h,
        http_version_t1,
        http_version_t2,
        http_version_p,
        http_version_slash,
        http_version_major_start,
        http_version_major,
        http_version_minor_start,
        http_version_minor,
        newline_1,
        header_line_start,
        header_lws,
        header_name,
        header_space,
        header_value,
        newline_2,
        newline_3,
    } state_;

    std::string method_;
    std::string uri_;
    int major_version_;
    int minor_version_;
    std::vector<Header> headers_;

    std::string buffer_;
    std::string value_buffer_;
};

QDebug operator<<(QDebug d, const RequestParser &parser);

#endif // REQUESTPARSER_H
