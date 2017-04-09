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
    request()
{

}

void Connection::start()
{
    do_read_client_request();
}

void Connection::stop()
{

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
            do_write_client_request();
        }
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
