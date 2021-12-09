// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2021 Benjamin Bader
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

#include "core/Transaction.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>

#include <QDebug>
#include <QPointer>

#include <asio.hpp>

#include "log/Log.h"

#include "core/common.h"
#include "core/Errors.h"

namespace ama {

namespace {

std::ostream& operator<<(std::ostream& os, NotificationState ns)
{
    switch (ns)
    {
    case NotificationState::None: return os << "NotificationState::None";
    case NotificationState::RequestHeaders: return os << "NotificationState::RequestHeaders";
    case NotificationState::RequestBody: return os << "NotificationState::RequestBody";
    case NotificationState::RequestComplete: return os << "NotificationState::RequestComplete";
    case NotificationState::ResponseHeaders: return os << "NotificationState::ResponseHeaders";
    case NotificationState::ResponseBody: return os << "NotificationState::ResponseBody";
    case NotificationState::ResponseComplete: return os << "NotificationState::ResponseComplete";
    case NotificationState::TLSTunnel: return os << "NotificationState::TLSTunnel";
    case NotificationState::Error: return os << "NotificationState::Error";
    default:
        assert(false);
        return os << "(unknown NotificationState: " << static_cast<uint8_t>(ns) << ")";
    }
}

class NotificationStateValue : public log::ILogValue
{
public:
    NotificationStateValue(NotificationState state)
        : state_(state)
    {}

    const char* name() const override
    {
        return "ns";
    }

    void accept(log::LogValueVisitor& visitor) const override
    {
        std::stringstream ss;
        ss << state_;
        visitor.visit(log::StringValue(name(), ss.str()));
    }

private:
    NotificationState state_;
};

class ParsePhaseValue : public log::ILogValue
{
public:
    ParsePhaseValue(ama::ParsePhase phase)
        : ParsePhaseValue("phase", phase)
    {}

    ParsePhaseValue(const char* name, ama::ParsePhase phase)
        : name_(name)
        , phase_(phase)
    {}

    const char* name() const override
    {
        return name_;
    }

    void accept(log::LogValueVisitor& visitor) const override
    {
        std::stringstream ss;
        ::operator<<(ss, phase_); // wtf, c++
        visitor.visit(log::StringValue(name(), ss.str()));
    }

private:
    const char* name_;
    ama::ParsePhase phase_;
};

} // namespace

Transaction::Transaction(int id, ConnectionPool* connectionPool, const std::shared_ptr<Conn>& clientConnection, QObject* parent)
    : QObject{parent}
    , id_{id}
    , error_{}
    , client_{clientConnection}
    , remote_{}
    , connection_pool_{connectionPool}
    , state_{TransactionState::Start}
    , parser_{}
    , read_buffer_{}
    , remote_buffer_{nullptr}
    , raw_input_{}
    , request_parse_phase_{ParsePhase::Start}
    , request_{}
    , response_parse_phase_{ParsePhase::Start}
    , response_{}
    , notification_state_{NotificationState::None}
{}

int Transaction::id() const
{
    return id_;
}

TransactionState Transaction::state() const
{
    return state_;
}

Request& Transaction::request()
{
    return request_;
}

Response& Transaction::response()
{
    return response_;
}

void Transaction::begin()
{
    emit on_transaction_start(this);
    raw_input_.clear();

    read_client_request();
}

void Transaction::read_client_request()
{
    log::debug("read_client_request()", log::IntValue("id", id_));

    QPointer<Transaction> self(this);
    client_->async_read_some(read_buffer_, [self](asio::error_code ec, size_t num_read)
    {
        if (self == nullptr)
        {
            return;
        }

        log::debug(
            "read_client_request#async_read_some",
            log::IntValue("id", self->id_),
            log::StringValue("ec", ec.message()),
            log::SizeValue("num_read", num_read)
        );

        if (ec == asio::error::eof)
        {
            // unexpected disconnect
            self->notify_failure(ProxyError::ClientDisconnected);
            return;
        }

        if (ec)
        {
            self->notify_failure(ec);
            return;
        }

        auto start = std::begin(self->read_buffer_);
        auto stop = start + num_read;

        auto current_phase = self->request_parse_phase_;
        auto state = self->parser_.parse(self->request(), start, stop, self->request_parse_phase_);
        while (state == HttpMessageParser::State::Incomplete && current_phase != self->request_parse_phase_)
        {
            log::debug(
                "read_client_request() (phase change)",
                ParsePhaseValue("old", current_phase),
                ParsePhaseValue("new", self->request_parse_phase_)
            );

            self->notify_phase_change(self->request_parse_phase_);

            current_phase = self->request_parse_phase_;
            state = self->parser_.parse(self->request(), start, stop, self->request_parse_phase_);
        }

        if (state == HttpMessageParser::State::Incomplete)
        {
            log::debug("read_client_request() (parse: Incomplete)", log::IntValue("id", self->id_));
            self->read_client_request();
            return;
        }
        else if (state == HttpMessageParser::State::Invalid)
        {
            log::debug("read_client_request() (parse: Invalid)", log::IntValue("id", self->id_));
            self->notify_failure(ProxyError::MalformedRequest);
        }
        else if (state == HttpMessageParser::State::Valid)
        {
            log::debug("read_client_request() (parse: Valid)", log::IntValue("id", self->id_));
            if (self->request().method() == "CONNECT")
            {
                log::debug("read_client_request() (do TLS tunnel)", log::IntValue("id", self->id_));
                self->establish_tls_tunnel();
            }
            else
            {
                log::debug("read_client_request() (do notify and relay request)", log::IntValue("id", self->id_));
                self->do_notification(NotificationState::RequestComplete);
                self->open_remote_connection();
            }
        }
        else
        {
            // wtf, this isn't any status we recognize
            self->notify_failure(ProxyError::NetworkError);
        }
    });
}

void Transaction::open_remote_connection()
{
    auto headerValues = request().headers().find_by_name("Host");
    if (headerValues.empty())
    {
        log::warn("open_remote_connection(): Malformed request - no 'Host' header found!", log::IntValue("id", id_));
        notify_failure(ProxyError::MalformedRequest);
        return;
    }

    std::string host = headerValues[0];
    std::string port = "80";

    size_t separator = host.find(':');
    if (separator != std::string::npos)
    {
        try
        {
            port = host.substr(separator + 1);
            host = host.substr(0, separator);
        }
        catch (std::out_of_range)
        {
            log::warn("open_remote_connection(): Malformed request - assuming port 80.", log::IntValue("id", id_));
        }
        catch (std::invalid_argument)
        {
            log::warn("open_remote_connection(): Malformed request - assuming port 80.", log::IntValue("id", id_));
        }
    }

    QPointer<Transaction> self(this);
    connection_pool_->try_open(host, port, [self](auto conn, auto ec)
    {
        if (self == nullptr)
        {
            return;
        }

        if (ec)
        {
            self->notify_failure(ec);
            return;
        }

        self->remote_ = conn;
        self->send_client_request_to_remote();
    });
}

void Transaction::send_client_request_to_remote()
{
    QPointer<Transaction> self(this);
    auto formatted_request = std::make_shared<std::string>(request_.format());
    remote_->async_write(asio::buffer(*formatted_request),
                         [self, formatted_request](auto ec, size_t num_bytes_written)
    {
        UNUSED(num_bytes_written);

        if (self == nullptr)
        {
            return;
        }

        if (ec)
        {
            self->notify_failure(ec);
            return;
        }

        self->raw_input_.clear();
        self->parser_.resetForResponse();
        self->read_remote_response();
    });
}

void Transaction::read_remote_response()
{
    QPointer<Transaction> self(this);
    remote_->async_read_some(read_buffer_, [self](auto ec, size_t num_bytes_read)
    {
        if (self == nullptr)
        {
            return;
        }

        if (ec == asio::error::eof)
        {
            // unexpected disconnect
            self->notify_failure(ProxyError::RemoteDisconnected);
            return;
        }

        if (ec)
        {
            // some other error
            self->notify_failure(ec);
            return;
        }

        auto begin = std::begin(self->read_buffer_);
        auto end = begin + num_bytes_read;

        std::copy(begin, end, std::back_inserter(self->raw_input_));

        auto current_phase = self->response_parse_phase_;
        auto state = self->parser_.parse(self->response(), begin, end, self->response_parse_phase_);
        while (state == HttpMessageParser::State::Incomplete && current_phase != self->response_parse_phase_)
        {
            log::debug("read_remote_response() (phase change)", ParsePhaseValue("old", current_phase), ParsePhaseValue("new", self->response_parse_phase_));
            self->notify_phase_change(self->response_parse_phase_);

            current_phase = self->response_parse_phase_;
            state = self->parser_.parse(self->response(), begin, end, self->response_parse_phase_);
        }

        switch (state)
        {
        case HttpMessageParser::State::Incomplete:
            self->read_remote_response();
            break;

        case HttpMessageParser::State::Invalid:
            self->notify_failure(ProxyError::MalformedResponse);
            break;

        case HttpMessageParser::Valid:
            self->do_notification(NotificationState::ResponseComplete);

            self->send_remote_response_to_client();
            break;

        default:
            // wtf
            self->notify_failure(ProxyError::MalformedResponse);
            break;
        }
    });
}

void Transaction::send_remote_response_to_client()
{
    QPointer<Transaction> self(this);
    client_->async_write(asio::buffer(raw_input_), [self](auto ec, size_t num_bytes_written)
    {
        UNUSED(num_bytes_written);

        if (self == nullptr)
        {
            return;
        }

        if (ec == asio::error::eof)
        {
            self->notify_failure(ec);
            return;
        }

        if (ec)
        {
            self->notify_failure(ec);
            return;
        }

        // We're done!
        self->complete_transaction();
    });
}

void Transaction::establish_tls_tunnel()
{
    std::string host = request().uri();
    std::string port = "443";

    size_t separator = host.find(':');
    if (separator != std::string::npos)
    {
        try
        {
            port = host.substr(separator + 1);
            host = host.substr(0, separator);
        }
        catch (std::out_of_range)
        {
            log::warn("establish_tls_tunnel(): Malformed request - assuming port 443.", log::IntValue("id", id_));
        }
        catch (std::invalid_argument)
        {
            log::warn("establish_tls_tunnel(): Malformed request - assuming port 443.", log::IntValue("id", id_));
        }
    }

    asio::ip::tcp::resolver::query query(host, port);

    QPointer<Transaction> self(this);
    connection_pool_->try_open(host, port, [self](auto conn, auto ec)
    {
        if (self == nullptr)
        {
            return;
        }

        bool success = true;
        if (ec)
        {
            success = false;
        }

        std::string responseText;
        if (success) {
            self->remote_ = conn;
            responseText = "HTTP/1.1 200 OK\r\n"
                           "Proxy-Agent: amanuensis 0.1.0\r\n"
                           "\r\n";
        } else {
            responseText = "HTTP/1.1 400 Bad Request\r\n"
                           "Proxy-Agent: amanuensis 0.1.0\r\n"
                           "\r\n";
        }

        auto responseBuffer = std::make_shared<std::string>(responseText);
        self->client_->async_write(asio::buffer(*responseBuffer),
                                   [self, ec, success, responseBuffer]
                                   (auto ec2, auto num_bytes_written)
        {
            UNUSED(num_bytes_written);

            if (self == nullptr)
            {
                return;
            }

            bool localSuccess = success;
            if (ec2)
            {
                // (double?) fail
                log::warn("Failed to send CONNECT reply to client", log::StringValue("what", ec2.message()));
                localSuccess = false;
            }

            if (localSuccess)
            {
                // Time to start acting like a dumb pipe.
                // We'll need a second buffer.
                self->remote_buffer_ = std::make_unique<std::array<uint8_t, 8192>>();

                self->send_client_request_via_tunnel();
                self->send_server_response_via_tunnel();
            }
            else
            {
                // Failed to establish a remote tunnel, so fail.
                self->notify_failure(ec);
            }
        });
    });
}

void Transaction::send_client_request_via_tunnel()
{
    if (client_ == nullptr || remote_ == nullptr)
    {
        return;
    }

    QPointer<Transaction> self(this);
    client_->async_read_some(read_buffer_, [self](auto ec, size_t num_bytes_read)
    {
        if (self == nullptr)
        {
            return;
        }

        if (ec == asio::error::eof || num_bytes_read == 0)
        {
            // finished normally?
            self->release_connections();
            return;
        }

        if (ec)
        {
            // finish abnormally.
            self->notify_failure(ec);
            return;
        }

        auto sendBuffer = asio::buffer(self->read_buffer_, num_bytes_read);
        self->remote_->async_write(sendBuffer, [self, num_bytes_read](auto ec, size_t num_bytes_written)
        {
            if (self == nullptr)
            {
                return;
            }

            if (ec)
            {
                // Fail
                self->notify_failure(ec);
                return;
            }

            if (num_bytes_read != num_bytes_written)
            {
                self->notify_failure(ProxyError::NetworkError);
                return;
            }

            // loop
            self->send_client_request_via_tunnel();
        });
    });
}

void Transaction::send_server_response_via_tunnel()
{
    if (client_ == nullptr || remote_ == nullptr)
    {
        return;
    }

    QPointer<Transaction> self(this);
    remote_->async_read_some(*remote_buffer_, [self](auto ec, size_t num_bytes_read)
    {
        if (self == nullptr)
        {
            return;
        }

        if (ec == asio::error::eof || num_bytes_read == 0)
        {
            // finished normally?
            self->release_connections();
            return;
        }

        if (ec)
        {
            // finish abnormally.
            self->notify_failure(ec);
            return;
        }

        auto sendBuffer = asio::buffer(*self->remote_buffer_, num_bytes_read);
        self->client_->async_write(sendBuffer, [self, num_bytes_read](auto ec, size_t num_bytes_written)
        {
            if (self == nullptr)
            {
                return;
            }

            if (ec)
            {
                // Fail
                self->notify_failure(ec);
                return;
            }

            if (num_bytes_read != num_bytes_written)
            {
                self->notify_failure(ProxyError::NetworkError);
                return;
            }

            // loop
            self->send_server_response_via_tunnel();
        });
    });
}

void Transaction::complete_transaction()
{
    release_connections();
    emit on_transaction_complete(this);
    deleteLater();
}

void Transaction::release_connections()
{
    client_.reset();
    remote_.reset();
}

void Transaction::notify_phase_change(ParsePhase phase)
{
    NotificationState ns;
    switch (phase)
    {
    case ParsePhase::Start:
        log::error("Should not notify on this phase", ParsePhaseValue(phase));
        assert(false);
        return;
    case ParsePhase::ReceivedMessageLine:
        // nothing to notify on
        return;
    case ParsePhase::ReceivedHeaders:
        ns = notification_state_ < NotificationState::RequestComplete
                ? NotificationState::RequestHeaders
                : NotificationState::ResponseHeaders;
        break;
    case ParsePhase::ReceivedBody:
        ns = notification_state_ < NotificationState::RequestComplete
                ? NotificationState::RequestBody
                : NotificationState::ResponseBody;
        break;
    case ParsePhase::ReceivedFullMessage:
        ns = notification_state_ < NotificationState::RequestComplete
                ? NotificationState::RequestComplete
                : NotificationState::ResponseComplete;
        break;
    default:
        log::error("Unexpected ParsePhase value", ParsePhaseValue(phase));
        assert(false);
        return;
    }

    do_notification(ns);
}

void Transaction::do_notification(NotificationState ns)
{
    log::debug("Transaction::do_notification", log::IntValue("tx", id_), NotificationStateValue(ns));
    while (notification_state_ < ns)
    {
        uint8_t ns_int = static_cast<uint8_t>(notification_state_);
        auto current_state = notification_state_;
        notification_state_ = static_cast<NotificationState>(ns_int + 1);
        log::verbose("Transaction::do_notification - one step", log::IntValue("id", id_), NotificationStateValue(current_state));
        switch (current_state)
        {
        case NotificationState::None:
            // nothing, yet
            break;
        case NotificationState::RequestHeaders:
            // also nothing, yet
            break;
        case NotificationState::RequestBody:
            // ditto
            break;
        case NotificationState::RequestComplete:
            emit on_request_read(this);
            break;
        case NotificationState::ResponseHeaders:
            emit on_response_headers_read(this);
            break;
        case NotificationState::ResponseBody:
            // nothing
            break;
        case NotificationState::ResponseComplete:
            emit on_response_read(this);
            emit on_transaction_complete(this);
            break;
        case NotificationState::TLSTunnel:
            // nothing
            break;
        case NotificationState::Error:
            log::error("Errors should use notify_failure, not do_notification");
            assert(false);
            break;
        }
    }
}

void Transaction::notify_failure(std::error_code ec)
{
    release_connections();

    error_ = ec;
    notification_state_ = NotificationState::Error;

    emit on_transaction_failed(this);

    complete_transaction();
}

} // namespace ama
