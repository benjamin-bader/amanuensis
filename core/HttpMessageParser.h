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

#ifndef HTTPMESSAGEPARSER_H
#define HTTPMESSAGEPARSER_H

#pragma once

#include <cstdint>
#include <string>
#include <iostream>
#include <system_error>
#include <tuple>
#include <vector>

#include "common.h"
#include "global.h"

#include "Headers.h"
#include "Request.h"
#include "Response.h"

class QDebug;

namespace ama
{

class HttpMessage;

/**
 * Enumerates the various phases of incremental HTTP message parsing.
 *
 * @par Parsing consists of stepping through a large number of different
 * states, many of which are logically related and can be grouped together
 * into a smaller number of phases. Parsing can, optionally, pause whenever
 * the current phase changes.  This allows actions to be taken at interesting
 * points, such as when all headers have been parsed but before reading a
 * potentially large entity.
 */
enum class ParsePhase : int
{
    Start = 0,
    ReceivedMessageLine,
    ReceivedHeaders,
    ReceivedBody,
    ReceivedFullMessage
};

class A_EXPORT HttpMessageParser
{

public:
    HttpMessageParser();
    ~HttpMessageParser();

    enum State {
        Incomplete = 0,
        Valid,
        Invalid
    };

    void resetForRequest();
    void resetForResponse();

    template <typename InputIterator>
    State parse(Request &request, InputIterator &begin, InputIterator end)
    {
        return parse(request.message_, begin, end);
    }

    template <typename InputIterator>
    State parse(Request &request, InputIterator &begin, InputIterator end, ParsePhase& phase)
    {
        return parse(request.message_, begin, end, phase);
    }

    template <typename InputIterator>
    State parse(Response &response, InputIterator &begin, InputIterator end)
    {
        return parse(response.message_, begin, end);
    }

    template <typename InputIterator>
    State parse(Response &response, InputIterator &begin, InputIterator end, ParsePhase& phase)
    {
        return parse(response.message_, begin, end, phase);
    }

    template <typename InputIterator>
    State parse(HttpMessage &message, InputIterator &begin, InputIterator end)
    {
        return parse(message, begin, end, nullptr);
    }

    template <typename InputIterator>
    State parse(HttpMessage &message, InputIterator &begin, InputIterator end, ParsePhase& phase)
    {
        return parse(message, begin, end, &phase);
    }

private:
    friend QDebug operator<<(QDebug, const HttpMessageParser &parser);

    template <typename InputIterator>
    State parse(HttpMessage &message, InputIterator &begin, InputIterator end, ParsePhase* phase);

    State consume(HttpMessage &message, char input, ParsePhase* phase);

private:
    enum ParserState {
        // Request status line
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

        // Response status line
        //

        response_start                   = 50,
        response_http_t1                 = 51,
        response_http_t2                 = 52,
        response_http_p                  = 53,
        response_http_slash              = 54,
        response_major_version_start     = 55,
        response_major_version           = 56,
        response_minor_version_start     = 57,
        response_minor_version           = 58,

        response_status_code_start       = 59,
        response_status_code             = 60,

        response_status_message_start    = 61,
        response_status_message          = 62,
        response_newline                 = 63,

        // Headers
        //

        header_line_start                = 100,
        header_lws                       = 101,
        header_name                      = 102,
        header_space                     = 103,
        header_value                     = 104,
        newline_2                        = 105,
        newline_3                        = 106,

        // Entities
        //

        // Chunked entities
        chunk_length_start               = 200,
        chunk_length                     = 201,
        chunk_extension                  = 202, // unsure of the format, TODO
        chunk_length_newline             = 203,

        chunk                            = 204,
        chunk_trailing_newline           = 205,
        chunk_sequence_terminating_cr    = 206,
        chunk_sequence_terminating_lf    = 207,
        chunk_sequence_terminating_cr_2  = 208,
        chunk_sequence_terminating_lf_2  = 209,
        chunk_trailing_header_line_start = 210,
        chunk_trailing_header_lws        = 211,
        chunk_trailing_header_name       = 212,
        chunk_trailing_header_space      = 213,
        chunk_trailing_header_value      = 214,
        chunk_terminating_newline        = 215,

        // Non-chunked entities
        fixed_length_entity              = 300,
    } state_;

    void transition_to_state(ParserState newState);

    static ParsePhase get_phase_for_state_transition(ParsePhase phase, ParserState oldState, ParserState newState);

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

template <typename InputIterator>
HttpMessageParser::State HttpMessageParser::parse(HttpMessage &message, InputIterator &begin, InputIterator end, ParsePhase* phase)
{
    ParsePhase startingPhase = ParsePhase::Start;
    if (phase != nullptr)
    {
        startingPhase = *phase;
    }

    while (begin != end)
    {
        auto state = consume(message, *begin++, phase);
        if (state != State::Incomplete)
        {
            if (state == State::Valid && phase != nullptr)
            {
                *phase = ParsePhase::ReceivedFullMessage;
            }
            return state;
        }

        if (phase != nullptr && *phase != startingPhase)
        {
            break;
        }
    }

    return Incomplete;
}

} // namespace ama

std::ostream& operator<<(std::ostream& os, ama::ParsePhase phase);

#endif // HTTPMESSAGEPARSER_H
