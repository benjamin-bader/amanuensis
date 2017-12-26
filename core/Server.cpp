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
#include <memory>
#include <thread>

#include <QDebug>

#include <asio.hpp>

#include "ConnectionPool.h"
#include "ProxyTransaction.h"

using namespace ama;

class Server::impl
{
public:
    impl(int port) :
        port_(port),
        io_service_(),
        signals_(io_service_),
        acceptor_(io_service_),
        socket_(io_service_),
        workers_(),
        connection_pool_(std::make_shared<ConnectionPool>(io_service_))
    {}

    int port_;
    asio::io_service io_service_;
    asio::signal_set signals_;
    asio::ip::tcp::acceptor acceptor_;
    asio::ip::tcp::socket socket_;

    std::vector<std::thread> workers_;

    std::shared_ptr<ConnectionPool> connection_pool_;
};

Server::Server(const int port) :
    impl_(std::make_unique<Server::impl>(port))
{
    impl_->signals_.add(SIGINT);
    impl_->signals_.add(SIGTERM);
#if defined(SIGQUIT)
    impl_->signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)

    impl_->signals_.async_wait([this](std::error_code /*ec*/, int /*signo*/) {
       impl_->acceptor_.close();
    });

    asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);

    impl_->acceptor_.open(endpoint.protocol());
    impl_->acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    impl_->acceptor_.bind(endpoint);
    impl_->acceptor_.listen();

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
        auto thread = std::thread([this] { impl_->io_service_.run(); });
        impl_->workers_.push_back(std::move(thread));
    }
}

Server::~Server()
{
    impl_->signals_.clear();
    impl_->acceptor_.close();
    impl_->io_service_.stop();

    impl_->connection_pool_ = nullptr;

    std::for_each(impl_->workers_.begin(), impl_->workers_.end(), [](std::thread &t) {
        if (t.joinable())
        {
            t.join();
        }
    });

    impl_ = nullptr;
}

std::shared_ptr<ConnectionPool> Server::connection_pool() const
{
    return impl_->connection_pool_;
}

void Server::do_accept()
{
    impl_->acceptor_.async_accept(impl_->socket_, [this] (asio::error_code ec) {
        if (!impl_->acceptor_.is_open())
        {
            qDebug() << "Acceptor has closed, abandoning accept";
            return;
        }

        if (!ec)
        {
            impl_->connection_pool_->make_connection(std::move(impl_->socket_));
            do_accept();
        }
        else
        {
            qWarning() << "Server::do_accept caught: " << QString(ec.message().c_str());
        }
    });
}
