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

#include "RequestParser.h"

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include <QDebug>

#include "Headers.h"
#include "Request.h"
#include "Response.h"

class ResponseBuilder {};

#if defined(Q_OS_WIN)
#define Q_STRTOULL _strtoui64
#else
#define Q_STRTOULL strtoull
#endif

namespace
{
    bool parse_uint64_t(const std::string &str, uint64_t &result, int base = 10)
    {
        char *end;
        auto parsed = Q_STRTOULL(str.c_str(), &end, base);

        auto parse_error = errno;
        if (parsed == 0 && (parse_error == ERANGE || parse_error == EINVAL))
        {
            return false;
        }

        result = parsed;
        return true;
    }

    bool ci_equal(const std::string &lhs, const std::string &rhs)
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }

        for (size_t i = 0; i < lhs.size(); ++i)
        {
            if (std::tolower(lhs[i]) != std::tolower(rhs[i]))
            {
                return false;
            }
        }

        return true;
    }

    inline bool is_char(int input)
    {
        return input >= 0 && input <= 127;
    }

    inline bool is_ctl(int input)
    {
        return (input >= 0 && input <= 31) || (input == 127);
    }

    inline bool is_tspecial(int input)
    {
        switch (input)
        {
        case '(':
        case ')':
        case '<':
        case '@':
        case ',':
        case ';':
        case ':':
        case '\\':
        case '"':
        case '/':
        case '[':
        case ']':
        case '?':
        case '=':
        case '{':
        case '}':
        case ' ':
        case '\t':
            return true;

        default:
            return false;
        }
    }

    inline bool is_digit(int input)
    {
        return input >= '0' && input <= '9';
    }

    inline bool is_hex(int input)
    {
        return is_digit(input)
                || (input >= 'a' && input <= 'f')
                || (input >= 'A' && input <= 'F');
    }

    inline int hex_value(char c)
    {
        if (is_digit(c))
        {
            return c - '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            return c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F')
        {
            return c - 'A' + 10;
        }
        else
        {
            qFatal("Not a hex digit: %c", c);
            return -1;
        }
    }
}

RequestParser::RequestParser() :
    state_(method_start),
    method_(),
    uri_(),
    major_version_(0),
    minor_version_(0),
    headers_(),
    remaining_(0),
    buffer_(),
    value_buffer_()
{
    buffer_.reserve(64);
    value_buffer_.reserve(64);
}

RequestParser::~RequestParser()
{
    // here to satisfy my wonky build setup
}

void RequestParser::reset()
{
    state_ = method_start;
    method_.clear();
    uri_.clear();
    major_version_ = 0;
    minor_version_ = 0;
    headers_.clear();
    remaining_ = 0;
    buffer_.clear();
    value_buffer_.clear();
}

void RequestParser::transition_to_state(ParserState newState)
{
    state_ = newState;
}

RequestParser::State RequestParser::consume(Request &request, char input)
{
#if defined(DEBUG_PARSER_TRANSITIONS)
#define TRANSIT(x) do { \
    qDebug() << state_ << "->" << (x) << "(" #x ")"; \
    transition_to_state((x)); \
} while (false)
#else
#define TRANSIT(x) transition_to_state((x))
#endif

    switch (state_)
    {
    case method_start:
        if (!is_char(input) || is_ctl(input) || is_tspecial(input))
        {
            return Invalid;
        }

        TRANSIT(method);
        request.method_.push_back(input);
        return Incomplete;

    case method:
        if (input == ' ')
        {
            TRANSIT(uri);
            return Incomplete;
        }
        else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
        {
            return Invalid;
        }
        else
        {
            request.method_.push_back(input);
            return Incomplete;
        }

    case uri:
        if (input == ' ')
        {
            TRANSIT(http_version_h);
            return Incomplete;
        }
        else if (is_ctl(input))
        {
            return Invalid;
        }
        else
        {
            request.uri_.push_back(input);
            return Incomplete;
        }

    case http_version_h:
        if (input == 'H'){
            TRANSIT(http_version_t1);
            return Incomplete;
        }
        return Invalid;

    case http_version_t1:
        if (input == 'T')
        {
            TRANSIT(http_version_t2);
            return Incomplete;
        }
        return Invalid;

    case http_version_t2:
        if (input == 'T')
        {
            TRANSIT(http_version_p);
            return Incomplete;
        }
        return Invalid;

    case http_version_p:
        if (input == 'P')
        {
            TRANSIT(http_version_slash);
            return Incomplete;
        }
        return Invalid;

    case http_version_slash:
        if (input == '/')
        {
            TRANSIT(http_version_major_start);
            request.major_version_ = 0;
            request.minor_version_ = 0;
            return Incomplete;
        }
        return Invalid;

    case http_version_major_start:
        if (is_digit(input))
        {
            TRANSIT(http_version_major);
            request.major_version_ = input - '0';
            return Incomplete;
        }
        return Invalid;

    case http_version_major:
        if (input == '.')
        {
            TRANSIT(http_version_minor_start);
            return Incomplete;
        }
        else if (is_digit(input))
        {
            request.major_version_ *= 10;
            request.major_version_ += input - '0';
            return Incomplete;
        }
        return Invalid;

    case http_version_minor_start:
        if (is_digit(input))
        {
            TRANSIT(http_version_minor);
            request.minor_version_ = input - '0';
            return Incomplete;
        }
        return Invalid;

    case http_version_minor:
        if (input == '\r')
        {
            TRANSIT(newline_1);
            return Incomplete;
        }
        else if (is_digit(input))
        {
            request.minor_version_ *= 10;
            request.minor_version_ += input - '0';
            return Incomplete;
        }
        return Invalid;

    case newline_1:
        if (input == '\n')
        {
            TRANSIT(header_line_start);
            return Incomplete;
        }
        return Invalid;

    case header_line_start:
        if (input == '\r')
        {
            TRANSIT(newline_3);
            return Incomplete;
        }
        else if (!request.headers_.empty() && (input == ' ' || input == '\t'))
        {
            TRANSIT(header_lws);
            return Incomplete;
        }
        else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
        {
            return Invalid;
        }
        else
        {
            TRANSIT(header_name);
            buffer_.clear();
            buffer_.push_back(input);
            return Incomplete;
        }

    case header_lws:
        if (input == '\r')
        {
            TRANSIT(newline_2);
            return Incomplete;
        }
        else if (input == ' ' || input == '\t')
        {
            return Incomplete;
        }
        else if (is_ctl(input))
        {
            return Invalid;
        }
        else
        {
            TRANSIT(header_value);
            return Incomplete;
        }

    case header_name:
        if (input == ':')
        {
            TRANSIT(header_space);
            return Incomplete;
        }
        else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
        {
            return Invalid;
        }
        else
        {
            buffer_.push_back(input);
            return Incomplete;
        }

    case header_space:
        if (input == ' ')
        {
            TRANSIT(header_value);
            value_buffer_.clear();
            return Incomplete;
        }
        return Invalid;

    case header_value:
        if (input == '\r')
        {
            TRANSIT(newline_2);
            request.headers_.push_back(Header(std::move(buffer_), std::move(value_buffer_)));
            return Incomplete;
        }
        else if (! is_ctl(input))
        {
            value_buffer_.push_back(input);
            return Incomplete;
        }
        return Invalid;

    case newline_2:
        if (input == '\n')
        {
            TRANSIT(header_line_start);
            return Incomplete;
        }
        return Invalid;

    case newline_3:
        if (input == '\n')
        {
            auto iter = request.headers_.find_by_name("Transfer-Encoding");
            if (iter != request.headers_.end())
            {
                // TODO(ben): This doesn't account for more than one value,
                // which is explicitly allowed in the 1.1 spec.

                if (ci_equal(iter->value(), "chunked"))
                {
                    TRANSIT(chunk_length_start);
                    request.body_.clear();
                    return Incomplete;
                }
                else if (! ci_equal(iter->value(), "identity"))
                {
                    qFatal("Unsupported chunk encoding: %s", iter->value().c_str());

                    // qFatal aborts the program, but just in case
                    return Invalid;
                }

                // fall through, check for content length
            }

            iter = request.headers_.find_by_name("Content-Length");
            if (iter != request.headers_.end())
            {
                uint64_t length;
                if (! parse_uint64_t(iter->value(), length))
                {
                    return Invalid;
                }

                TRANSIT(fixed_length_entity);
                remaining_ = length;
                request.body_.clear();
                request.body_.reserve(static_cast<size_t>(length));
                return Incomplete;
            }

            // No entity expected, we're done!
            return Valid;
        }
        return Invalid;

    case chunk_length_start:
        if (input == '0')
        {
            TRANSIT(chunk_sequence_terminating_cr);
            return Incomplete;
        }
        else if (is_hex(input))
        {
            TRANSIT(chunk_length);
            remaining_ = hex_value(input);
            return Incomplete;
        }
        return Invalid;

    case chunk_length:
        if (input == '\r')
        {
            TRANSIT(chunk_length_newline);
            return Incomplete;
        }
        else if (is_hex(input))
        {
            remaining_ = (remaining_ * 16) + hex_value(input);
            return Incomplete;
        }
        else if (input == ';')
        {
            qFatal("extensions not implemented");
        }
        return Invalid;

    case chunk_length_newline:
        if (input == '\n')
        {
            TRANSIT(chunk);
            return Incomplete;
        }
        return Invalid;

    case chunk:
        if (remaining_ == 0)
        {
            if (input == '\r')
            {
                TRANSIT(chunk_trailing_newline);
                return Incomplete;
            }
            return Invalid;
        }
        else
        {
            request.body_.push_back(input);
            remaining_--;
            return Incomplete;
        }

    case chunk_trailing_newline:
        if (input == '\n')
        {
            TRANSIT(chunk_length_start);
            return Incomplete;
        }
        return Invalid;

    case chunk_sequence_terminating_cr:
        if (input == '\r')
        {
            TRANSIT(chunk_sequence_terminating_lf);
            return Incomplete;
        }
        return Invalid;

    case chunk_sequence_terminating_lf:
        if (input == '\n')
        {
            TRANSIT(chunk_trailing_header_line_start);
            return Incomplete;
        }
        return Invalid;

    case chunk_trailing_header_line_start:
        if (input == '\r')
        {
            TRANSIT(chunk_terminating_newline);
            return Incomplete;
        }
        qFatal("Trailing headers not implemented");
        return Invalid;

    case chunk_terminating_newline:
        if (input == '\n')
        {
            return Valid;
        }
        return Invalid;

    case fixed_length_entity:
        request.body_.push_back(static_cast<uint8_t>(input));
        --remaining_;

        if (remaining_ == 0)
        {
            return Valid;
        }
        else
        {
            return Incomplete;
        }

    default:
        qFatal("Unimplemented state: %d", state_);
        return Invalid;
    }

    return Invalid;
}

QDebug operator<<(QDebug d, const RequestParser &parser)
{
    return d << "RequestParser{state="
             << parser.state_
             << ", buffer=" << QString(parser.buffer_.c_str())
             << ", value_buffer= " << QString(parser.value_buffer_.c_str())
             << "}";
}
