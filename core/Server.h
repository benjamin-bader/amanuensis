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

#include <memory>
#include <thread>
#include <vector>

#include <asio/io_service.hpp>
#include <asio/signal_set.hpp>
#include <asio/ip/tcp.hpp>

#include "global.h"

class ConnectionManager;

class A_EXPORT Server : public std::enable_shared_from_this<Server>
{
public:
    Server(const int port = 9999);
    ~Server();

    std::shared_ptr<ConnectionManager> connection_manager() const;

private:
    void do_accept();

    int port_;
    asio::io_service io_service_;
    asio::signal_set signals_;
    asio::ip::tcp::acceptor acceptor_;
    asio::ip::tcp::socket socket_;

    std::vector<std::thread> workers_;

    std::shared_ptr<ConnectionManager> connectionManager_;
};

#endif // SERVER_H
