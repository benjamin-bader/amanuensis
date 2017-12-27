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

#include <ctime>

#include <chrono>
#include <cstdint>
#include <iomanip>
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

using namespace ama;

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

private:
    void read_client_request();
    void open_remote_connection();
    void send_client_request_to_remote();

    void read_remote_response();
    void send_remote_response_to_client();

    void establish_tls_tunnel();
    void send_client_request_via_tunnel();
    void send_server_response_via_tunnel();

    void notify_failure(std::error_code ec);

    void release_connections();

private:
    int id_;
    std::error_code error_;

    std::shared_ptr<Conn> client_;
    std::shared_ptr<Conn> remote_;

    std::shared_ptr<ConnectionPool> connection_pool_;

    TransactionState state_;

    HttpMessageParser parser_;
    std::shared_ptr<ProxyTransaction> parent_;

    std::array<uint8_t, 8192> read_buffer_;
    std::unique_ptr<std::array<uint8_t, 8192>> remote_buffer_;

    // Records the exact bytes received from client/server, so that
    // they can be relayed as-is.
    std::vector<uint8_t> raw_input_;

    Request request_;
    Response response_;
};

ProxyTransaction::impl::impl(
        int id,
        std::shared_ptr<ConnectionPool> connectionPool,
        std::shared_ptr<Conn> clientConnection)
    : id_(id)
    , error_()
    , client_(clientConnection)
    , remote_(nullptr)
    , connection_pool_(connectionPool)
    , state_(TransactionState::Start)
    , parser_()
    , parent_(nullptr)
    , read_buffer_()
    , remote_buffer_(nullptr)
    , raw_input_()
    , request_()
    , response_()
{
}

ProxyTransaction::impl::~impl()
{
    qDebug() << "ProxyTransaction::impl::~impl() id=" << id_;
}

void ProxyTransaction::impl::begin(std::shared_ptr<ProxyTransaction> parent)
{
    parent_ = parent;
    parent_->notify_listeners([this](auto &listener)
    {
        listener->on_transaction_start(*parent_);
    });

    raw_input_.clear();
    read_client_request();
}

void ProxyTransaction::impl::read_client_request()
{
    qDebug() << "ProxyTransaction::impl::read_client_request() id=" << id_;

    auto self = shared_from_this();
    client_->async_read_some(read_buffer_, [self](asio::error_code ec, size_t num_read)
    {
        qDebug() << "ProxyTransaction::impl::read_client_request()#async_read_some id=" << self->id_ << " ec=" << ec.message().c_str() << " num_read=" << num_read;
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

        self->raw_input_.insert(self->raw_input_.end(), start, stop);

        auto state = self->parser_.parse(self->request(), start, stop);
        if (state == HttpMessageParser::State::Incomplete)
        {
            qDebug() << "ProxyTransaction::impl::read_client_request() (parse: Incomplete) id=" << self->id_;
            self->read_client_request();
            return;
        }
        else if (state == HttpMessageParser::State::Invalid)
        {
            qDebug() << "ProxyTransaction::impl::read_client_request() (parse: Invalid) id=" << self->id_;
            self->notify_failure(ProxyError::MalformedRequest);
        }
        else if (state == HttpMessageParser::State::Valid)
        {
            qDebug() << "ProxyTransaction::impl::read_client_request() (parse: Valid) id=" << self->id_;
            if (self->request().method() == "CONNECT")
            {
                qDebug() << "ProxyTransaction::impl::read_client_request() (do TLS tunnel) id=" << self->id_;
                self->establish_tls_tunnel();
            }
            else
            {
                qDebug() << "ProxyTransaction::impl::read_client_request() (do notify and relay request) id=" << self->id_;
                self->parent_->notify_listeners([self](auto &listener)
                {
                    listener->on_request_read(*self->parent_);
                });

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
        qWarning() << "Malformed request - no 'Host' header found!";
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
            qWarning() << "Malformed request - assuming port 80";
        }
        catch (std::invalid_argument)
        {
            qWarning() << "Malformed request - assuming port 80";
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
    remote_->async_write(asio::buffer(raw_input_), [self](auto ec, size_t num_bytes_written)
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
    auto self = shared_from_this();
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

       self->raw_input_.insert(self->raw_input_.end(), begin, end);

       switch (self->parser_.parse(self->response(), begin, end))
       {
       case HttpMessageParser::State::Incomplete:
           self->read_remote_response();
           break;

       case HttpMessageParser::State::Invalid:
           self->notify_failure(ProxyError::MalformedResponse);
           break;

       case HttpMessageParser::Valid:
           self->parent_->notify_listeners([self](auto &listener)
           {
               listener->on_response_read(*self->parent_);
           });

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
        self->parent_->notify_listeners([self](auto &listener)
        {
            listener->on_transaction_complete(*self->parent_);
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
            qWarning() << "Malformed request - assuming port 443";
        }
        catch (std::invalid_argument)
        {
            qWarning() << "Malformed request - assuming port 443";
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
                qWarning() << "Failed to send CONNECT reply to client: " << ec2.message().c_str();
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

void ProxyTransaction::impl::notify_failure(std::error_code error)
{
    release_connections();

    error_ = error;
    parent_->notify_listeners([this](auto &listener)
    {
        listener->on_transaction_failed(*parent_);
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

