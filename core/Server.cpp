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

#include <QDebug>

#include "ConnectionPool.h"
#include "ProxyTransaction.h"

namespace ama {

Server::Server(const int port)
    : port_(port)
    , io_service_()
    , signals_(io_service_)
    , acceptor_(io_service_)
    , socket_(io_service_)
    , workers_()
    , connection_pool_(std::make_shared<ConnectionPool>(io_service_))
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

    // We will multiplex running the io_service across multiple threads.
    // The number of threads ideally will be one less than the STL's
    // self-reported hardware_concurrency amount, so that the main thread
    // remains free even if we're getting slammed with requests.  Practically
    // speaking, the threads will be asleep 99% of the time, waiting on IO.
    //
    // std::thread::hardware_concurrency() is documented to return 0 if it
    // cannot settle on a good number.  If it does, we'll assume a value of
    // four - dual-core with hyperthreading is a low bar to meet in 2017.

    int numSupportedThreads = static_cast<int>(std::thread::hardware_concurrency());
    if (numSupportedThreads == 0)
    {
        numSupportedThreads = 4;
    }

    numSupportedThreads = std::max(numSupportedThreads - 1, 4);

    for (int i = 0; i < numSupportedThreads; ++i)
    {
        auto thread = std::thread([&] { io_service_.run(); });
        workers_.push_back(std::move(thread));
    }
}

Server::~Server()
{
    signals_.clear();
    acceptor_.close();
    io_service_.stop();

    connection_pool_ = nullptr;

    std::for_each(workers_.begin(), workers_.end(), [](std::thread &t) {
        if (t.joinable())
        {
            t.join();
        }
    });

    workers_.clear();
}

std::shared_ptr<ConnectionPool> Server::connection_pool() const
{
    return connection_pool_;
}

void Server::do_accept()
{
    acceptor_.async_accept(socket_, [this] (asio::error_code ec) {
        if (!acceptor_.is_open())
        {
            qDebug() << "Acceptor has closed, abandoning accept";
            return;
        }

        if (!ec)
        {
            connection_pool_->make_connection(std::move(socket_));
            do_accept();
        }
        else
        {
            qWarning() << "Server::do_accept caught: " << QString(ec.message().c_str());
        }
    });
}

} // ama
