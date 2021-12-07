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

#include "ConnectionPool.h"

using namespace ama;

using tcp = asio::ip::tcp;

Conn::Conn(asio::io_context& ctx)
        : socket_(ctx)
        , expires_at_(time_point::max())
        , should_close_(false)
    {}

Conn::Conn(tcp::socket&& socket)
    : socket_(std::move(socket))
    , expires_at_(time_point::max())
    , should_close_(false)
{}

Conn::~Conn()
{
    socket_.close();
}

ConnectionPool::~ConnectionPool()
{

}

std::shared_ptr<Conn> ConnectionPool::make_connection(asio::ip::tcp::socket &&socket)
{
    auto connection = std::make_shared<Conn>(std::move(socket));
    notify_listeners([connection](auto &listener)
    {
        listener->on_client_connected(connection);
    });
    return connection;
}

std::shared_ptr<Conn> ConnectionPool::find_open_connection(const std::string &host, int port)
{
    UNUSED(host);
    UNUSED(port);
    return std::shared_ptr<Conn>(nullptr);
}

void ConnectionPool::try_open(const std::string &host, const std::string &port, std::function<void (std::shared_ptr<Conn>, std::error_code)> &&callback)
{
    auto self = shared_from_this();
    auto conn = std::make_shared<Conn>(context_);

    tcp::resolver::query query(host, port);

    resolver_.async_resolve(query, [this, self, conn, callback]
                            (asio::error_code ec, tcp::resolver::iterator result)
    {
        if (ec)
        {
            callback(nullptr, ec);
            return;
        }

        asio::async_connect(conn->socket_, result,
                            [this, self, conn, callback]
                            (asio::error_code ec, tcp::resolver::iterator /* i */)
        {
            if (ec)
            {
                callback(nullptr, ec);
                return;
            }

            callback(conn, ec);
        });
    });
}
