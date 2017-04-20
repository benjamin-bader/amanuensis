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

#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <set>
#include <string>

#include "global.h"

#include "asiofwd.h"
#include "Listenable.h"
#include "ObjectPool.h"

class Connection;

typedef std::array<char, 8192> BufferType;
typedef ObjectPool<BufferType> BufferPool;
typedef BufferPool::pool_ptr BufferPtr;

class A_EXPORT ConnectionManagerListener
{
public:
    virtual void on_connected(const std::shared_ptr<Connection> &connection) = 0;
};

class A_EXPORT ConnectionManager : public Listenable<ConnectionManagerListener>
{
public:
    ConnectionManager(asio::io_service &io_service);
    ~ConnectionManager();

    void start(std::shared_ptr<Connection> connection);

    void stop(std::shared_ptr<Connection> connection);

    void stop_all();

    asio::ip::basic_resolver<asio::ip::tcp, asio::ip::resolver_service<asio::ip::tcp>>& resolver();

    // Returns a shared pointer to a buffer.  When its refcount reaches
    // zero, the buffer will be returned to a shared pool.
    BufferPtr takeBuffer();

private:
    ConnectionManager(const ConnectionManager &) = delete;
    ConnectionManager& operator =(const ConnectionManager &) = delete;

    class impl;
    const std::unique_ptr<impl> impl_;
};

#endif // CONNECTIONMANAGER_H
