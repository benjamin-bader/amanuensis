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

#include "core/ConnectionPool.h"

#include "AsioConnection.h"

using namespace ama;

using tcp = asio::ip::tcp;

ConnectionPool::ConnectionPool(asio::io_context& context, QObject* parent)
    : QObject{parent}
    , context_(context)
    , resolver_(context)
{}

ConnectionPool::~ConnectionPool()
{

}

std::shared_ptr<IConnection> ConnectionPool::make_connection(asio::ip::tcp::socket &&socket)
{
    auto connection = std::make_shared<TcpConnection>(std::move(socket));
    emit client_connected(connection);
    return connection;
}

std::shared_ptr<IConnection> ConnectionPool::find_open_connection(const std::string &host, int port)
{
    (void) host;
    (void) port;
    return std::shared_ptr<IConnection>(nullptr);
}

void ConnectionPool::try_open(const std::string &host, const std::string &port, OpenCallback&& callback)
{
    auto conn = std::make_shared<TcpConnection>(asio::ip::tcp::socket(context_));

    tcp::resolver::query query(host, port);

    resolver_.async_resolve(query, [conn, callback = std::move(callback)]
                            (asio::error_code ec, tcp::resolver::iterator result)
    {
        if (ec)
        {
            callback(nullptr, ec);
            return;
        }

        asio::async_connect(conn->socket(), result,
                            [conn, callback = std::move(callback)]
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
