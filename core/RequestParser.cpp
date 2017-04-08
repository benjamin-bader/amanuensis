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

#include <QDebug>

#include "Request.h"
#include "Response.h"

class ResponseBuilder {};

namespace
{
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
}

RequestParser::RequestParser()
{
    Request request;
    buffer_.reserve(64);
    value_buffer_.reserve(64);
}

void RequestParser::reset()
{
    buffer_.clear();
    value_buffer_.clear();
}

RequestParser::State RequestParser::consume(Request &request, char input)
{
    switch (state_)
    {
    case method_start:
        if (!is_char(input) || is_ctl(input) || is_tspecial(input))
        {
            return Invalid;
        }

        state_ = method;
        request.method_.push_back(input);
        return Incomplete;

    case method:
        if (input == ' ')
        {
            state_ = uri;
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
            state_ = http_version_h;
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
            state_ = http_version_t1;
            return Incomplete;
        }
        else
        {
            return Invalid;
        }

    case http_version_t1:
        if (input == 'T')
        {
            state_ = http_version_t2;
            return Incomplete;
        }
        else
        {
            return Invalid;
        }

    case http_version_t2:
        if (input == 'T')
        {
            state_ = http_version_p;
            return Incomplete;
        }
        else
        {
            return Invalid;
        }

    case http_version_p:
        if (input == 'P')
        {
            state_ = http_version_slash;
            return Incomplete;
        }
        else
        {
            return Invalid;
        }

    case http_version_slash:
        if (input == '/')
        {
            request.major_version_ = 0;
            request.minor_version_ = 0;
            state_ = http_version_major_start;
            return Incomplete;
        }
        else
        {
            return Invalid;
        }

    case http_version_major_start:
        if (is_digit(input))
        {
            request.major_version_ = input - '0';
            state_ = http_version_major;
            return Incomplete;
        }
        else
        {
            return Invalid;
        }

    case http_version_major:
        if (input == '.')
        {
            state_ = http_version_minor_start;
            return Incomplete;
        }
        else if (is_digit(input))
        {
            request.major_version_ *= 10;
            request.major_version_ += input - '0';
            return Incomplete;
        }
        else
        {
            return Invalid;
        }

    case http_version_minor_start:
        if (is_digit(input))
        {
            request.minor_version_ = input - '0';
            state_ = http_version_minor;
            return Incomplete;
        }
        else
        {
            return Invalid;
        }

    case http_version_minor:
        if (input == '\r')
        {
            state_ = newline_1;
            return Incomplete;
        }
        else if (is_digit(input))
        {
            request.minor_version_ *= 10;
            request.minor_version_ += input - '0';
            return Incomplete;
        }
        else
        {
            return Invalid;
        }

    case newline_1:
        if (input == '\n')
        {
            state_ = header_line_start;
            return Incomplete;
        }
        else
        {
            return Invalid;
        }

    case header_line_start:
        if (input == '\r')
        {
            state_ = newline_3;
            return Incomplete;
        }
        else if (!request.headers_.empty() && (input == ' ' || input == '\t'))
        {
            state_ = header_lws;
            return Incomplete;
        }
        else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
        {
            return Invalid;
        }
        else
        {
            buffer_.clear();
            buffer_.push_back(input);
            state_ = header_name;
            return Incomplete;
        }

    case header_lws:
        if (input == '\r')
        {
            state_ = newline_2;
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
            state_ = header_value;
            return Incomplete;
        }

    case header_name:
        if (input == ':')
        {
            state_ = header_space;
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
            value_buffer_.clear();
            state_ = header_value;
            return Incomplete;
        }
        else
        {
            return Invalid;
        }

    case header_value:
        if (input == '\r')
        {
            request.headers_.push_back(Header(std::move(buffer_), std::move(value_buffer_)));
            state_ = newline_2;
            return Incomplete;
        }
        else if (is_ctl(input))
        {
            return Invalid;
        }
        else
        {
            value_buffer_.push_back(input);
            return Incomplete;
        }

    case newline_2:
        if (input == '\n')
        {
            state_ = header_line_start;
            return Incomplete;
        }
        else
        {
            return Invalid;
        }

    case newline_3:
        if (input == '\n')
        {
            return Valid;
        }
        else
        {
            return Invalid;
        }

    default:
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
