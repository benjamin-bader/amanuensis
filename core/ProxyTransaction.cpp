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

#include "ProxyTransaction.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>

#include <QDebug>

#include <asio.hpp>

#include "common.h"

#include "ConnectionPool.h"
#include "Errors.h"
#include "HttpMessageParser.h"
#include "Logging.h"

#include "Log.h"

QDebug operator<<(QDebug d, ama::ParsePhase phase)
{
    std::stringstream ss;
    ss << phase;
    return d << ss.str().c_str();
}

namespace ama {

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

ProxyTransaction::ProxyTransaction(int id, std::shared_ptr<ConnectionPool> connectionPool, std::shared_ptr<Conn> clientConnection)
    : Transaction()
    , id_(id)
    , error_()
    , logger_(get_logger("ProxyTransaction"))
    , client_(clientConnection)
    , remote_(nullptr)
    , connection_pool_(connectionPool)
    , state_(TransactionState::Start)
    , parser_()
    , read_buffer_()
    , remote_buffer_(nullptr)
    , raw_input_()
    , request_parse_phase_(ParsePhase::Start)
    , request_()
    , response_parse_phase_(ParsePhase::Start)
    , response_()
    , notification_state_(NotificationState::None)
{
}

void ProxyTransaction::begin()
{
    notify_listeners([this](auto& listener)
    {
        listener->on_transaction_start(*this);
    });

    raw_input_.clear();
    read_client_request();
}

void ProxyTransaction::read_client_request()
{
    logger_->debug("read_client_request() id={}", id_);

    auto self = shared_from_this();
    client_->async_read_some(read_buffer_, [self](asio::error_code ec, size_t num_read)
    {
        self->logger_->debug("read_client_request()#async_read_some id={} ec={} num_read={}", self->id_, ec.message(), num_read);
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
            self->logger_->debug("read_client_request() (phase change) old={} new={}", current_phase, self->request_parse_phase_);
            self->notify_phase_change(self->request_parse_phase_);

            current_phase = self->request_parse_phase_;
            state = self->parser_.parse(self->request(), start, stop, self->request_parse_phase_);
        }

        if (state == HttpMessageParser::State::Incomplete)
        {
            self->logger_->debug("read_client_request() (parse: Incomplete) id={}", self->id_);
            self->read_client_request();
            return;
        }
        else if (state == HttpMessageParser::State::Invalid)
        {
            self->logger_->debug("read_client_request() (parse: Invalid) id={}", self->id_);
            self->notify_failure(ProxyError::MalformedRequest);
        }
        else if (state == HttpMessageParser::State::Valid)
        {
            self->logger_->debug("read_client_request() (parse: Valid) id={}", self->id_);
            if (self->request().method() == "CONNECT")
            {
                self->logger_->debug("read_client_request() (do TLS tunnel) id={}", self->id_);
                self->establish_tls_tunnel();
            }
            else
            {
                self->logger_->debug("read_client_request() (do notify and relay request) id={}", self->id_);
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

void ProxyTransaction::open_remote_connection()
{
    auto headerValues = request().headers().find_by_name("Host");
    if (headerValues.empty())
    {
        logger_->warn("open_remote_connection(): Malformed request - no 'Host' header found! id={}", id_);
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
            logger_->warn("open_remote_connection(): Malformed request - assuming port 80.  id={}", id_);
        }
        catch (std::invalid_argument)
        {
            logger_->warn("open_remote_connection(): Malformed request - assuming port 80.  id={}", id_);
        }
    }

    auto self = shared_from_this();
    connection_pool_->try_open(host, port, [self](auto conn, auto ec)
    {
        if (ec)
        {
            self->notify_failure(ec);
            return;
        }

        self->remote_ = conn;
        self->send_client_request_to_remote();
    });
}

void ProxyTransaction::send_client_request_to_remote()
{
    auto self = shared_from_this();
    auto formatted_request = std::make_shared<std::string>(request_.format());
    remote_->async_write(asio::buffer(*formatted_request),
                         [self, formatted_request](auto ec, size_t num_bytes_written)
    {
        UNUSED(num_bytes_written);

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

void ProxyTransaction::read_remote_response()
{
    std::shared_ptr<ProxyTransaction> self = shared_from_this();
    remote_->async_read_some(read_buffer_, [self](auto ec, size_t num_bytes_read)
    {
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
           self->logger_->debug("read_remote_response() (phase change) old={} new={}", current_phase, self->response_parse_phase_);
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

void ProxyTransaction::send_remote_response_to_client()
{
    auto self = shared_from_this();
    client_->async_write(asio::buffer(raw_input_), [self](auto ec, size_t num_bytes_written)
    {
        UNUSED(num_bytes_written);

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
        self->notify_listeners([self](auto& listener)
        {
            listener->on_transaction_complete(*self);
        });

        self->release_connections();
    });
}

void ProxyTransaction::establish_tls_tunnel()
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
            logger_->warn("Malformed request - assuming port 443 id={}", id_);
        }
        catch (std::invalid_argument)
        {
            logger_->warn("Malformed request - assuming port 443 id={}", id_);
        }
    }

    asio::ip::tcp::resolver::query query(host, port);

    auto self = shared_from_this();
    connection_pool_->try_open(host, port, [self](auto conn, auto ec)
    {
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

            bool localSuccess = success;
            if (ec2)
            {
                // (double?) fail
                self->logger_->warn("Failed to send CONNECT reply to client: {}", ec2.message());
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

void ProxyTransaction::send_client_request_via_tunnel()
{
    if (client_ == nullptr || remote_ == nullptr)
    {
        return;
    }

    auto self = shared_from_this();
    client_->async_read_some(read_buffer_, [self](auto ec, size_t num_bytes_read)
    {
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

void ProxyTransaction::send_server_response_via_tunnel()
{
    if (client_ == nullptr || remote_ == nullptr)
    {
        return;
    }

    auto self = shared_from_this();
    remote_->async_read_some(*remote_buffer_, [self](auto ec, size_t num_bytes_read)
    {
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

void ProxyTransaction::notify_phase_change(ParsePhase phase)
{
    NotificationState ns;
    switch (phase)
    {
    case ParsePhase::Start:
        logger_->error("Should not notify for {}", phase);
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
        logger_->critical("Unexpected ParsePhase value: {}", phase);
        assert(false);
        return;
    }

    do_notification(ns);
}

void ProxyTransaction::do_notification(NotificationState ns)
{
    logger_->debug("tx({}): do_notification(ns={})", id_, ns);
    while (notification_state_ < ns)
    {
        uint8_t ns_int = static_cast<uint8_t>(notification_state_);
        auto current_state = notification_state_;
        notification_state_ = static_cast<NotificationState>(ns_int + 1);

        ama::log::log_event(
                    ama::log::Severity::Verbose,
                    "sending tx notification",
                    ama::log::IntValue("tx", id_),
                    ama::log::U8Value("old state", ns_int),
                    ama::log::U8Value("new state", ns_int + 1));

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
            notify_listeners([this](auto& listener)
            {
                listener->on_request_read(*this);
            });
            break;
        case NotificationState::ResponseHeaders:
            notify_listeners([this](auto& listener)
            {
                listener->on_response_headers_read(*this);
            });
            break;
        case NotificationState::ResponseBody:
            // nothing
            break;
        case NotificationState::ResponseComplete:
            notify_listeners([this](auto& listener)
            {
                listener->on_response_read(*this);
                listener->on_transaction_complete(*this);
            });
            break;
        case NotificationState::TLSTunnel:
            // nothing
            break;
        case NotificationState::Error:
            logger_->critical("Errors should use notify_failure, not do_notification");
            assert(false);
            break;
        }
    }
}

void ProxyTransaction::notify_failure(std::error_code error)
{
    release_connections();

    error_ = error;
    notification_state_ = NotificationState::Error;
    notify_listeners([this](auto& listener)
    {
        listener->on_transaction_failed(*this);
    });
}

void ProxyTransaction::release_connections()
{
    client_.reset();
    remote_.reset();
}

} // namespace ama
