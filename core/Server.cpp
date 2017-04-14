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

#include "Server.h"

#include <algorithm>
#include <thread>

#include <QDebug>

#include "Connection.h"
#include "ConnectionManager.h"

Server::Server(const int port) :
    port_(port),
    io_service_(),
    signals_(io_service_),
    acceptor_(io_service_),
    socket_(io_service_),
    workers_(),
    connectionManager_(std::make_shared<ConnectionManager>(io_service_))
{
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
#if defined(SIGQUIT)
    signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)

    signals_.async_wait([this](std::error_code /*ec*/, int /*signo*/) {
       acceptor_.close();
    });

    asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    do_accept();

    for (int i = 0; i < 8; ++i)
    {
        auto thread = std::thread([this] { io_service_.run(); });
        workers_.push_back(std::move(thread));
    }
}

Server::~Server()
{
    acceptor_.close();
    signals_.clear();
    io_service_.stop();

    connectionManager_->stop_all();

    std::for_each(workers_.begin(), workers_.end(), [](std::thread &t) {
        if (t.joinable())
        {
            t.join();
        }
    });
}

std::shared_ptr<ConnectionManager> Server::connection_manager() const
{
    return connectionManager_;
}

void Server::do_accept()
{
    acceptor_.async_accept(socket_, [this] (asio::error_code ec) {
        if (!acceptor_.is_open())
        {
            qDebug() << "Acceptor has closed, abandoning accept";
        }

        if (!ec)
        {
            qDebug() << "Accepted a client connection, processing it";
            connectionManager_->start(std::make_shared<Connection>(std::move(socket_), connectionManager_));

            do_accept();
        }
        else
        {
            qWarning() << "Awww!  " << QString(ec.message().c_str());
        }
    });
}
