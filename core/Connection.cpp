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

#include "Connection.h"

#include <QDebug>

#include <asio.hpp>

#include "ConnectionManager.h"

#include "Headers.h"
#include "Request.h"
#include "Response.h"

// doesn't exist yet
//class ResponseBuilder {};

namespace {
    QDebug operator<<(QDebug d, const std::string &str)
    {
        d << QString(str.c_str());
        return d;
    }

    void dump_request(const Request &req)
    {
        qDebug() << req.method() << req.uri() << " " << req.major_version() << "/" << req.minor_version();

        for (const Header& header : req.headers())
        {
            qDebug() << header.name() << ": " << header.value();
        }
    }
}

Connection::Connection(asio::ip::tcp::socket socket, std::shared_ptr<ConnectionManager> connectionManager) :
    socket_(std::move(socket)),
    remoteSocket_(socket_.get_io_service()),
    connectionManager_(connectionManager),
    buffer_(),
    requestParser(),
    request()
{

}

void Connection::start()
{
    do_read_client_request();
}

void Connection::stop()
{
    asio::error_code ec;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    remoteSocket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
}

void Connection::do_read_client_request()
{
    auto self(shared_from_this());
    socket_.async_read_some(asio::buffer(buffer_), [this, self](asio::error_code ec, size_t bytes_read) {
        auto begin = buffer_.begin();
        auto end = begin + bytes_read;
        auto parserState = requestParser.parse(request, begin, end);

        if (parserState == RequestParser::State::Incomplete)
        {
            do_read_client_request();
        }
        else if (parserState == RequestParser::State::Invalid)
        {
            // d'oh, handle the error somehow
        }
        else
        {
            dump_request(request);

            lookup_remote_host();
        }
    });
}

void Connection::lookup_remote_host()
{
    auto iter = request.headers().find_by_name("Host");
    if (iter == request.headers().end())
    {
        qWarning() << "Malformed request - no 'Host' header found!";
        connectionManager_->stop(shared_from_this());
        return;
    }

    std::string host = iter->value();
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

    asio::ip::tcp::resolver::query query(host, port);

    auto self = shared_from_this();
    connectionManager_->resolver().async_resolve(query, [this, self](asio::error_code ec, asio::ip::tcp::resolver::iterator result) {
        if (ec)
        {
            // Something happened - couldn't connect to the remote endpoint?
            // TODO(ben): Propagate the failure!

            qWarning() << "Failed to connect to the remote endpoint: " << ec.message();
            connectionManager_->stop(self);
            return;
        }

        qInfo() << "DNS lookup complete, connecting to remote host";
        connect_to_remote_server(result);
    });
}

void Connection::connect_to_remote_server(asio::ip::tcp::resolver::iterator result)
{
    auto self = shared_from_this();
    asio::async_connect(remoteSocket_, result, [this, self](asio::error_code ec, asio::ip::tcp::resolver::iterator i) {
        if (ec)
        {
            qWarning() << "Failed to connect to remote host: " << ec.message();
            connectionManager_->stop(self);
            return;
        }

        qInfo() << "Connected to remote host " << i->host_name() << " at " << i->endpoint().address().to_string();

        {
            std::lock_guard<std::mutex> lock(outboxMutex_);
            outbox_.push(std::make_shared<std::vector<uint8_t>>(request.make_buffer()));
            if (! request.body().empty())
            {
                outbox_.push(std::make_shared<std::vector<uint8_t>>(request.body()));
            }
        }
        do_write_client_request();
    });
}

void Connection::do_write_client_request()
{
    std::shared_ptr<std::vector<uint8_t>> sharedBuffer;

    {
        std::lock_guard<std::mutex> lock(outboxMutex_);
        if (outbox_.empty())
        {
            return;
        }

        sharedBuffer = outbox_.front();
    }

    auto self = shared_from_this();
    asio::async_write(remoteSocket_,
                      asio::buffer(sharedBuffer->data(), sharedBuffer->size()),
                      [this, self, sharedBuffer](asio::error_code ec, size_t bytesWritten) {
        if (ec)
        {
            // boo
            qWarning() << "Error sending client request to server: " << ec.message();
            connectionManager_->stop(self);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(outboxMutex_);
            outbox_.pop();
        }

        do_write_client_request();
        do_read_server_response();
    });
}

void Connection::do_read_server_response()
{
    auto self = shared_from_this();
    remoteSocket_.async_read_some(asio::buffer(buffer_), [this, self](asio::error_code ec, size_t bytesRead) {
        std::string str(buffer_.data(), bytesRead);
        qDebug() << "Response: \n" << str;
    });
}

void Connection::do_write_server_response()
{

}
