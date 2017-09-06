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

#include "ConnectionManager.h"

#include <algorithm>

#include <asio.hpp>

#include "Connection.h"

using namespace ama;

class ConnectionManager::impl
{
public:
    impl(asio::io_service &io_service) :
        connections_(),
        mutex_(),
        resolver_(io_service),
        bufferPool_(),
        nextId_(1)
    {}

    ~impl() = default;

    std::set<std::shared_ptr<Connection>> connections_;
    std::mutex mutex_; // protects connections_

    asio::ip::tcp::resolver resolver_;

    BufferPool bufferPool_;

    int nextId_;
};

ConnectionManager::ConnectionManager(asio::io_service &io_service) :
    impl_(std::make_unique<ConnectionManager::impl>(io_service))
{

}

ConnectionManager::~ConnectionManager()
{
    stop_all();
    impl_->resolver_.cancel();
}

asio::ip::tcp::resolver& ConnectionManager::resolver()
{
    return impl_->resolver_;
}

void ConnectionManager::start(std::shared_ptr<Connection> connection)
{
    {
        std::lock_guard<std::mutex> lock(impl_->mutex_);
        int id = impl_->nextId_++;
        connection->set_id(id);

        impl_->connections_.insert(connection);
    }

    notify_listeners([&connection](const std::shared_ptr<ConnectionManagerListener> &listener) {
        listener->on_connected(connection);
    });

    connection->start();
}

void ConnectionManager::stop(std::shared_ptr<Connection> connection)
{
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->connections_.erase(connection);
    connection->stop();
}

void ConnectionManager::stop_all()
{
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    for (auto connection : impl_->connections_)
    {
        connection->stop();
    }
    impl_->connections_.clear();
}

BufferPtr ConnectionManager::takeBuffer()
{
    return impl_->bufferPool_.acquire();
}

