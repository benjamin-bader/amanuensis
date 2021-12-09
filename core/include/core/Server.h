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

#ifndef SERVER_H
#define SERVER_H

#pragma once

#include "core/global.h"

#include <QObject>

#include <memory>
#include <thread>
#include <vector>

#include <asio.hpp>

namespace ama
{

class ConnectionPool;
class Conn;

class A_EXPORT Server : public QObject
{
    Q_OBJECT

public:
    Server(const int port = 9999, QObject* parent = nullptr);
    ~Server();

    ConnectionPool* connection_pool() const;

signals:
    void connection_established(const std::shared_ptr<Conn>& conn);

private:
    void do_accept();

private:
    int port_;
    asio::io_context io_context_;
    asio::signal_set signals_;
    asio::ip::tcp::acceptor acceptor_;
    asio::ip::tcp::socket socket_;

    std::vector<std::thread> workers_;

    ConnectionPool* connection_pool_;
};

} // namespace ama

#endif // SERVER_H
