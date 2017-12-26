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

Conn::Conn(tcp::socket &&socket)
    : socket_(std::move(socket))
    , expires_at_(time_point::max())
    , should_close_(false)
{}

Conn::Conn(asio::io_service &service)
    : socket_(service)
    , expires_at_(time_point::max())
    , should_close_(false)
{}

Conn::~Conn()
{
    socket_.close();
}

class ConnectionPool::impl : public std::enable_shared_from_this<ConnectionPool::impl>
{
public:
    impl(asio::io_service &, ConnectionPool *);

    std::shared_ptr<Conn> make_connection(tcp::socket &&socket);

    std::shared_ptr<Conn> find_open_connection(const std::string &host, int port);

    void try_open(const std::string &host, const std::string &port, std::function<void (std::shared_ptr<Conn>, std::error_code)> &&callback);

private:
    tcp::resolver resolver_;
    ConnectionPool *pool_;
}; // class ConnectionPool::impl

ConnectionPool::impl::impl(asio::io_service &service, ConnectionPool *pool)
    : resolver_(service)
    , pool_(pool)
{

}

std::shared_ptr<Conn> ConnectionPool::impl::make_connection(tcp::socket &&socket)
{
    auto connection = std::make_shared<Conn>(std::move(socket));
    pool_->notify_listeners([connection](auto &listener)
    {
        listener->on_client_connected(connection);
    });
    return connection;
}

std::shared_ptr<Conn> ConnectionPool::impl::find_open_connection(const std::string &host, int port)
{
    UNUSED(host);
    UNUSED(port);
    return std::shared_ptr<Conn>(nullptr);
}

void ConnectionPool::impl::try_open(const std::string &host, const std::string &port, std::function<void (std::shared_ptr<Conn>, std::error_code)> &&callback)
{
    auto self = shared_from_this();
    auto conn = std::make_shared<Conn>(resolver_.get_io_service());

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

ConnectionPool::ConnectionPool(asio::io_service &service)
    : impl_(std::make_shared<ConnectionPool::impl>(service, this))
{}

ConnectionPool::~ConnectionPool()
{

}

std::shared_ptr<Conn> ConnectionPool::make_connection(asio::ip::tcp::socket &&socket)
{
    return impl_->make_connection(std::move(socket));
}

std::shared_ptr<Conn> ConnectionPool::find_open_connection(const std::string &host, int port)
{
    return impl_->find_open_connection(host, port);
}

void ConnectionPool::try_open(const std::string &host, const std::string &port, std::function<void (std::shared_ptr<Conn>, std::error_code)> &&callback)
{
    impl_->try_open(host, port, std::move(callback));
}
