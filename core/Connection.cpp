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

Connection::Connection(asio::ip::tcp::socket socket) :
    socket_(std::move(socket)),
    buffer_(),
    requestParser(),
    request(),
    resolver_(socket_.get_io_service()),
    remote_socket_(socket.get_io_service())
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
    remote_socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    resolver_.cancel();
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

            lookup_host();
        }
    });
}

void Connection::lookup_host()
{
    auto iter = request.headers().find_by_name("Host");
    if (iter == request.headers().end())
    {
        qWarning() << "Malformed request - no 'Host' header found!";
        stop();
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

    asio::ip::tcp::resolver::query q(host, port);

    auto self = shared_from_this();
    resolver_.async_resolve(q, [this, self](asio::error_code ec, asio::ip::tcp::resolver::iterator result) {
        if (ec)
        {
            // Something happened - couldn't connect to the remote endpoint?
            // TODO(ben): Propagate the failure!

            qWarning() << "Failed to connect to the remote endpoint: " << ec.message();
            stop();
            return;
        }

        asio::async_connect(remote_socket_, result, [this, self](asio::error_code ec2, asio::ip::tcp::resolver::iterator i) {
            // here
            qInfo() << "Connected to the remote server at " << (*i).host_name() << i->service_name();

            stop();
            return;
        });
    });
}

void Connection::do_write_client_request()
{
    //socket_.async_send()
}

void Connection::do_read_server_response()
{

}

void Connection::do_write_server_response()
{

}
