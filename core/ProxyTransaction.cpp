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

#include <array>
#include <cassert>
#include <chrono>
#include <ctime>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <vector>

#include <QDebug>

#include <asio.hpp>

#include "common.h"
#include "date.h"

#include "ConnectionPool.h"
#include "Errors.h"
#include "HttpMessageParser.h"
#include "Logging.h"

QDebug operator<<(QDebug d, ama::ParsePhase phase)
{
    std::stringstream ss;
    ss << phase;
    return d << ss.str().c_str();
}

namespace ama {

namespace {

enum class NotificationState : uint8_t
{
    None = 0,
    RequestHeaders = 1,
    RequestBody = 2,
    RequestComplete = 3,
    ResponseHeaders = 4,
    ResponseBody = 5,
    ResponseComplete = 6,

    TLSTunnel = 7,

    Error = 8,
};

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

} // namespace

class ProxyTransaction::impl : public std::enable_shared_from_this<ProxyTransaction::impl>
{
public:
    impl(int id, std::shared_ptr<ConnectionPool> connectionPool, std::shared_ptr<Conn> clientConnection);
    ~impl();

    int id() const { return id_; }
    TransactionState state() const { return state_; }
    std::error_code error() const { return error_; }

    void begin(std::shared_ptr<ProxyTransaction> parent);

    Request& request() { return request_; }
    Response& response() { return response_; }

    std::shared_ptr<spdlog::logger> logger() { return logger_; }

private:
    void read_client_request();
    void open_remote_connection();
    void send_client_request_to_remote();

    void read_remote_response();
    void send_remote_response_to_client();

    void establish_tls_tunnel();
    void send_client_request_via_tunnel();
    void send_server_response_via_tunnel();

    void notify_phase_change(ParsePhase phase);
    void do_notification(NotificationState ns);
    void notify_failure(std::error_code ec);

    void notify_listeners(const std::function<void(const std::shared_ptr<ProxyTransaction>&, const std::shared_ptr<TransactionListener>&)>& action);

    void release_connections();

private:
    int id_;
    std::error_code error_;

    std::shared_ptr<spdlog::logger> logger_;

    std::shared_ptr<Conn> client_;
    std::shared_ptr<Conn> remote_;

    std::shared_ptr<ConnectionPool> connection_pool_;

    TransactionState state_;

    HttpMessageParser parser_;
    std::weak_ptr<ProxyTransaction> parent_;

    std::array<uint8_t, 8192> read_buffer_;
    std::unique_ptr<std::array<uint8_t, 8192>> remote_buffer_;

    // Records the exact bytes received from client/server, so that
    // they can be relayed as-is.
    std::vector<uint8_t> raw_input_;

    ParsePhase request_parse_phase_;
    Request request_;

    ParsePhase response_parse_phase_;
    Response response_;

    NotificationState notification_state_;
};

ProxyTransaction::impl::impl(
        int id,
        std::shared_ptr<ConnectionPool> connectionPool,
        std::shared_ptr<Conn> clientConnection)
    : id_(id)
    , error_()
    , logger_(get_logger("ProxyTransaction"))
    , client_(clientConnection)
    , remote_(nullptr)
    , connection_pool_(connectionPool)
    , state_(TransactionState::Start)
    , parser_()
    , parent_()
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

ProxyTransaction::impl::~impl()
{
    logger()->debug("dtor() id={}", id_);
}

void ProxyTransaction::impl::begin(std::shared_ptr<ProxyTransaction> parent)
{
    parent_ = parent;
    notify_listeners([this](auto& parent, auto& listener)
    {
        listener->on_transaction_start(*parent);
    });

    raw_input_.clear();
    read_client_request();
}

void ProxyTransaction::impl::read_client_request()
{
    logger()->debug("read_client_request() id={}", id_);

    auto self = shared_from_this();
    client_->async_read_some(read_buffer_, [self](asio::error_code ec, size_t num_read)
    {
        self->logger()->debug("read_client_request()#async_read_some id={} ec={} num_read={}", self->id_, ec.message(), num_read);
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
            self->logger()->debug("read_client_request() (phase change) old={} new={}", current_phase, self->request_parse_phase_);
            self->notify_phase_change(self->request_parse_phase_);

            current_phase = self->request_parse_phase_;
            state = self->parser_.parse(self->request(), start, stop, self->request_parse_phase_);
        }

        if (state == HttpMessageParser::State::Incomplete)
        {
            self->logger()->debug("read_client_request() (parse: Incomplete) id={}", self->id_);
            self->read_client_request();
            return;
        }
        else if (state == HttpMessageParser::State::Invalid)
        {
            self->logger()->debug("read_client_request() (parse: Invalid) id={}", self->id_);
            self->notify_failure(ProxyError::MalformedRequest);
        }
        else if (state == HttpMessageParser::State::Valid)
        {
            self->logger()->debug("read_client_request() (parse: Valid) id={}", self->id_);
            if (self->request().method() == "CONNECT")
            {
                self->logger()->debug("read_client_request() (do TLS tunnel) id={}", self->id_);
                self->establish_tls_tunnel();
            }
            else
            {
                self->logger()->debug("read_client_request() (do notify and relay request) id={}", self->id_);
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

void ProxyTransaction::impl::open_remote_connection()
{
    auto headerValues = request().headers().find_by_name("Host");
    if (headerValues.empty())
    {
        logger()->warn("open_remote_connection(): Malformed request - no 'Host' header found! id={}", id_);
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
            logger()->warn("open_remote_connection(): Malformed request - assuming port 80.  id={}", id_);
        }
        catch (std::invalid_argument)
        {
            logger()->warn("open_remote_connection(): Malformed request - assuming port 80.  id={}", id_);
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

void ProxyTransaction::impl::send_client_request_to_remote()
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

void ProxyTransaction::impl::read_remote_response()
{
    std::shared_ptr<ProxyTransaction::impl> self = shared_from_this();
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
           self->logger()->debug("read_remote_response() (phase change) old={} new={}", current_phase, self->response_parse_phase_);
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

void ProxyTransaction::impl::send_remote_response_to_client()
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
        self->notify_listeners([self](auto& parent, auto& listener)
        {
            listener->on_transaction_complete(*parent);
        });

        self->release_connections();
    });
}

void ProxyTransaction::impl::establish_tls_tunnel()
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
            logger()->warn("Malformed request - assuming port 443 id={}", id_);
        }
        catch (std::invalid_argument)
        {
            logger()->warn("Malformed request - assuming port 443 id={}", id_);
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
                self->logger()->warn("Failed to send CONNECT reply to client: {}", ec2.message());
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

void ProxyTransaction::impl::send_client_request_via_tunnel()
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

void ProxyTransaction::impl::send_server_response_via_tunnel()
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

void ProxyTransaction::impl::notify_phase_change(ParsePhase phase)
{
    NotificationState ns;
    switch (phase)
    {
    case ParsePhase::Start:
        logger()->error("Should not notify for {}", phase);
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
        logger()->critical("Unexpected ParsePhase value: {}", phase);
        assert(false);
        return;
    }

    do_notification(ns);
}

void ProxyTransaction::impl::do_notification(NotificationState ns)
{
    logger()->debug("tx({}): do_notification(ns={})", id_, ns);
    while (notification_state_ < ns)
    {
        uint8_t ns_int = static_cast<uint8_t>(notification_state_);
        auto current_state = notification_state_;
        notification_state_ = static_cast<NotificationState>(ns_int + 1);
        SPDLOG_TRACE(logger(), "tx({}): notifying {}", id_, current_state);
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
            notify_listeners([this](auto& parent, auto listener)
            {
                listener->on_request_read(*parent);
            });
            break;
        case NotificationState::ResponseHeaders:
            notify_listeners([this](auto& parent, auto listener)
            {
                listener->on_response_headers_read(*parent);
            });
            break;
        case NotificationState::ResponseBody:
            // nothing
            break;
        case NotificationState::ResponseComplete:
            notify_listeners([this](auto& parent, auto listener)
            {
                listener->on_response_read(*parent);
                listener->on_transaction_complete(*parent);
            });
            break;
        case NotificationState::TLSTunnel:
            // nothing
            break;
        case NotificationState::Error:
            logger()->critical("Errors should use notify_failure, not do_notification");
            assert(false);
            break;
        }
    }
}


void ProxyTransaction::impl::notify_listeners(const std::function<void(const std::shared_ptr<ProxyTransaction>&, const std::shared_ptr<TransactionListener>&)>& action)
{
    using namespace std::placeholders;

    if (auto p = parent_.lock())
    {
        std::function<void(const std::shared_ptr<TransactionListener>&)> fn = std::bind(action, p, _1);
        p->notify_listeners(fn);
    }
}

void ProxyTransaction::impl::notify_failure(std::error_code error)
{
    release_connections();

    error_ = error;
    notification_state_ = NotificationState::Error;
    notify_listeners([](auto& parent, auto listener)
    {
        listener->on_transaction_failed(*parent);
    });
}

void ProxyTransaction::impl::release_connections()
{
    client_.reset();
    remote_.reset();
}

/////////////////////////////////

ProxyTransaction::ProxyTransaction(int id, std::shared_ptr<ConnectionPool> connectionPool, std::shared_ptr<Conn> clientConnection)
    : Transaction()
    , impl_(std::make_shared<impl>(id, connectionPool, clientConnection))
{
}

int ProxyTransaction::id() const
{
    return impl_->id();
}

TransactionState ProxyTransaction::state() const
{
    return impl_->state();
}

std::error_code ProxyTransaction::error() const
{
    return impl_->error();
}

Request& ProxyTransaction::request()
{
    return impl_->request();
}

Response& ProxyTransaction::response()
{
    return impl_->response();
}

void ProxyTransaction::begin()
{
    impl_->begin(shared_from_this());
}

time_point ProxyTransaction::parse_http_date(const std::string &text)
{
    // per RFC 7231, we MUST accept dates in the following formats:
    // 1. IMF-fixdate (e.g. Sun, 06 Nov 1994 08:49:37 GMT)
    // 2. RFC 859     (e.g. Sunday, 06-Nov-94 08:49:37 GMT)
    // 3. asctime()   (e.g. Sun Nov  6 08:49:37 1994)

    // see std::get_time for format-string documentation
    const std::string IMF_FIXDATE = "%a, %d %b %Y %H:%M:%S GMT";
    const std::string RFC_850     = "%a, %d-%b-%y %H:%M:%S GMT";
    const std::string ASCTIME     = "%a %b %d %H:%M:%S %Y";

    for (auto &format : { IMF_FIXDATE, RFC_850, ASCTIME })
    {
        time_point tp = {};
        std::istringstream input(text);

        input >> date::parse(format, tp);

        if (input)
        {
            return tp;
        }
    }

    throw std::invalid_argument("Invalid date-time value");
}

} // namespace ama
