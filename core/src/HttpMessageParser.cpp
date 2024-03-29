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

#include "core/HttpMessageParser.h"

#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include <QDebug>
#include <QString>
#include <QStringView>

#include "core/Headers.h"
#include "core/HttpMessage.h"

namespace ama {

#if defined(Q_OS_WIN)
#define Q_STRTOULL _strtoui64
#else
#define Q_STRTOULL strtoull
#endif

namespace {

bool parse_uint64_t(const std::string &str, uint64_t &result, int base = 10)
{
    char *end;

    errno = 0;
    auto parsed = Q_STRTOULL(str.c_str(), &end, base);

    auto parse_error = errno;
    if (parsed == 0 && (parse_error == ERANGE || parse_error == EINVAL))
    {
        return false;
    }

    result = parsed;
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
        std::stringstream ss;
        ss << "Not a hex value: " << c;
        throw std::domain_error(ss.str());
    }
}

} // anonymous namespace

HttpMessageParser::HttpMessageParser() :
    state_(method_start),
    remaining_(0),
    buffer_(),
    value_buffer_()
{
    buffer_.reserve(64);
    value_buffer_.reserve(64);
}

HttpMessageParser::~HttpMessageParser()
{
    // here to satisfy my wonky build setup
}

void HttpMessageParser::resetForRequest()
{
    state_ = method_start;
    remaining_ = 0;
    buffer_.clear();
    value_buffer_.clear();
}

void HttpMessageParser::resetForResponse()
{
    state_ = response_start;
    remaining_ = 0;
    buffer_.clear();
    value_buffer_.clear();
}

void HttpMessageParser::transition_to_state(ParserState newState)
{
    state_ = newState;
}

HttpMessageParser::State HttpMessageParser::consume(HttpMessage &message, char input, ParsePhase* phase)
{
#define TRANSIT(x) do { \
    if (phase != nullptr) { \
        *phase = get_phase_for_state_transition(*phase, state_, (x)); \
    } \
    transition_to_state((x)); \
} while (false)

    switch (state_)
    {
    case method_start:
        if (!is_char(input) || is_ctl(input) || is_tspecial(input))
        {
            return Invalid;
        }

        TRANSIT(method);
        message.method_.push_back(input);
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
            message.method_.push_back(input);
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
            message.uri_.push_back(input);
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
            message.major_version_ = 0;
            message.minor_version_ = 0;
            return Incomplete;
        }
        return Invalid;

    case http_version_major_start:
        if (is_digit(input))
        {
            TRANSIT(http_version_major);
            message.major_version_ = input - '0';
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
            message.major_version_ *= 10;
            message.major_version_ += input - '0';
            return Incomplete;
        }
        return Invalid;

    case http_version_minor_start:
        if (is_digit(input))
        {
            TRANSIT(http_version_minor);
            message.minor_version_ = input - '0';
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
            message.minor_version_ *= 10;
            message.minor_version_ += input - '0';
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

    case response_start:
        if (input == 'H')
        {
            TRANSIT(response_http_t1);
            return Incomplete;
        }
        return Invalid;

    case response_http_t1:
        if (input == 'T')
        {
            TRANSIT(response_http_t2);
            return Incomplete;
        }
        return Invalid;

    case response_http_t2:
        if (input == 'T')
        {
            TRANSIT(response_http_p);
            return Incomplete;
        }
        return Invalid;

    case response_http_p:
        if (input == 'P')
        {
            TRANSIT(response_http_slash);
            return Incomplete;
        }
        return Invalid;

    case response_http_slash:
        if (input == '/')
        {
            TRANSIT(response_major_version_start);
            return Incomplete;
        }
        return Invalid;

    case response_major_version_start:
        if (is_digit(input))
        {
            TRANSIT(response_major_version);
            message.major_version_ = input - '0';
            return Incomplete;
        }
        return Invalid;

    case response_major_version:
        if (input == '.')
        {
            TRANSIT(response_minor_version_start);
            return Incomplete;
        }
        else if (is_digit(input))
        {
            message.major_version_ = (message.major_version_ * 10) + (input - '0');
            return Incomplete;
        }
        return Invalid;

    case response_minor_version_start:
        if (is_digit(input))
        {
            TRANSIT(response_minor_version);
            message.minor_version_ = input - '0';
            return Incomplete;
        }
        return Invalid;

    case response_minor_version:
        if (input == ' ')
        {
            TRANSIT(response_status_code_start);
            return Incomplete;
        }
        else if (is_digit(input))
        {
            message.minor_version_ = (message.minor_version_ * 10) + (input - '0');
            return Incomplete;
        }
        return Invalid;

    case response_status_code_start:
        if (is_digit(input))
        {
            TRANSIT(response_status_code);
            message.status_code_ = input - '0';
            return Incomplete;
        }
        return Invalid;

    case response_status_code:
        if (input == ' ')
        {
            TRANSIT(response_status_message_start);
            return Incomplete;
        }
        else if (is_digit(input))
        {
            message.status_code_ = (message.status_code_ * 10) + (input - '0');
            return Incomplete;
        }
        return Invalid;

    case response_status_message_start:
        if (is_char(input))
        {
            TRANSIT(response_status_message);
            buffer_.clear();
            buffer_.push_back(input);
            return Incomplete;
        }
        return Invalid;

    case response_status_message:
        if (input == '\r')
        {
            TRANSIT(response_newline);
            message.status_message_ = buffer_;
            return Incomplete;
        }
        else if (is_char(input) || input == ' ')
        {
            buffer_.push_back(input);
            return Incomplete;
        }
        return Invalid;

    case response_newline:
        if (input == '\n')
        {
            TRANSIT(header_line_start);
            return Incomplete;
        }
        return Invalid;

    // Headers
    //

    case header_line_start:
        if (input == '\r')
        {
            TRANSIT(newline_3);
            return Incomplete;
        }
        else if (!message.headers_.empty() && (input == ' ' || input == '\t'))
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
            QString name = QString::fromLatin1(buffer_);
            QString value = QString::fromLatin1(value_buffer_);
            message.headers_.insert(name, value);

            buffer_.clear();
            value_buffer_.clear();

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
            auto headerValues = message.headers_.find_by_name("Transfer-Encoding");
            for (auto &value : headerValues)
            {
                bool is_chunked = false;

                // Is this a simple chunk stream?  If not, do we have a comma-separated list
                // of encodings, one of which might be 'chunked'?
                QStringView dataView(value);

                for (const auto& token : dataView.split(','))
                {
                    if (token.trimmed() == QStringLiteral("chunked"))
                    {
                        is_chunked = true;
                        break;
                    }
                }

                if (is_chunked)
                {
                    TRANSIT(chunk_length_start);
                    message.body_.clear();
                    return Incomplete;
                }
            }

            headerValues = message.headers_.find_by_name("Content-Length");
            for (auto &value : headerValues)
            {
                bool ok = false;
                auto length = value.toULongLong(&ok);
                if (! ok)
                {
                    return Invalid;
                }

                if (length == 0)
                {
                    return Valid;
                }

                TRANSIT(fixed_length_entity);
                remaining_ = length;
                message.body_.clear();
                message.body_.reserve(static_cast<size_t>(length));  // TODO: can length be bigger than a size_t?
                return Incomplete;
            }

            // No entity expected, we're done!
            return Valid;
        }
        return Invalid;

    case chunk_length_start:
        // RFC 7230 (s) 4.1.1. defines the terminal chunk length
        // as being exactly "0" (that is, only one zero), we
        // are more lenient here due to Github issue #23, where
        // we encountered servers that seem to prefix extra zeroes
        // to chunk lengths.
        if (is_hex(input))
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
            if (remaining_ > 0)
            {
                TRANSIT(chunk);
            }
            else
            {
                TRANSIT(chunk_trailing_header_line_start);
            }
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
            message.body_.push_back(input);
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
        message.body_.push_back(static_cast<uint8_t>(input));
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

ParsePhase HttpMessageParser::get_phase_for_state_transition(
        ParsePhase currentPhase,
        HttpMessageParser::ParserState oldState,
        HttpMessageParser::ParserState newState)
{
    if (oldState == newState)
    {
        return currentPhase;
    }

    switch (newState)
    {
    case header_line_start:
        if (currentPhase == ParsePhase::Start)
        {
            return ParsePhase::ReceivedMessageLine;
        }
        break;

    case chunk_length_start:
    case fixed_length_entity:
        if (currentPhase == ParsePhase::ReceivedMessageLine)
        {
            return ParsePhase::ReceivedHeaders;
        }
        break;

    default:
        // no-op
        break;
    }

    return currentPhase;
}

} // namespace ama

std::ostream& operator<<(std::ostream& os, ama::ParsePhase phase)
{
    switch (phase)
    {
    case ama::ParsePhase::Start:               return os << "ParsePhase::Start";
    case ama::ParsePhase::ReceivedMessageLine: return os << "ParsePhase::ReceivedMessageLine";
    case ama::ParsePhase::ReceivedHeaders:     return os << "ParsePhase::ReceivedHeaders";
    case ama::ParsePhase::ReceivedBody:        return os << "ParsePhase::ReceivedBody";
    case ama::ParsePhase::ReceivedFullMessage: return os << "ParsePhase::ReceivedFullMessage";
    default:
        assert(false);
        return os << "(unexpected phase value: " << static_cast<int>(phase) << ")";
    }
}

//QDebug operator<<(QDebug d, const HttpMessageParser &parser)
//{
//    return d << "RequestParser{state="
//             << parser.state_
//             << ", buffer=" << QString(parser.buffer_.c_str())
//             << ", value_buffer= " << QString(parser.value_buffer_.c_str())
//             << "}";
//}
