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

#include "global.h"

#include "Headers.h"

class QDebug;
class Request;

class A_EXPORT RequestParser
{

public:
    RequestParser();
    ~RequestParser();

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
        // Status line
        //

        method_start                     =  0,
        method                           =  1,
        uri                              =  2,
        http_version_h                   =  3,
        http_version_t1                  =  4,
        http_version_t2                  =  5,
        http_version_p                   =  6,
        http_version_slash               =  7,
        http_version_major_start         =  8,
        http_version_major               =  9,
        http_version_minor_start         = 10,
        http_version_minor               = 11,
        newline_1                        = 12,

        // Headers
        //

        header_line_start                = 13,
        header_lws                       = 14,
        header_name                      = 15,
        header_space                     = 16,
        header_value                     = 17,
        newline_2                        = 18,
        newline_3                        = 19,

        // Entities
        //

        // Chunked entities
        chunk_length_start               = 20,
        chunk_length                     = 21,
        chunk_extension                  = 22, // unsure of the format, TODO
        chunk_length_newline             = 23,

        chunk                            = 24,
        chunk_trailing_newline           = 25,
        chunk_sequence_terminating_cr    = 26,
        chunk_sequence_terminating_lf    = 27,
        chunk_sequence_terminating_cr_2  = 28,
        chunk_sequence_terminating_lf_2  = 29,
        chunk_trailing_header_line_start = 30,
        chunk_trailing_header_lws        = 31,
        chunk_trailing_header_name       = 32,
        chunk_trailing_header_space      = 33,
        chunk_trailing_header_value      = 34,
        chunk_terminating_newline        = 35,

        // Non-chunked entities
        fixed_length_entity              = 40,
    } state_;

    void transition_to_state(ParserState newState);

    std::string method_;
    std::string uri_;
    int major_version_;
    int minor_version_;
    std::vector<Header> headers_;

    // A counter of how many bytes in a fixed-length range
    // remain to be read; this is used both for individual
    // chunks as well as fixed-length entities.
    uint64_t remaining_;

    // A general-purpose string buffer, used for header names.
    std::string buffer_;

    // A special string buffer used for header values, so that
    // we keep the current header name at the same time.
    std::string value_buffer_;
};

QDebug operator<<(QDebug d, const RequestParser &parser);

#endif // REQUESTPARSER_H
