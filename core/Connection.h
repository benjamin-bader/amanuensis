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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>

#include <QObject>

#include <asio/ip/tcp.hpp>

#include "ConnectionManager.h"
#include "HttpMessage.h"
#include "HttpMessageParser.h"

class ConnectionManager;

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    Connection() = delete;
    Connection& operator=(const Connection&) = delete;

    explicit Connection(asio::ip::tcp::socket socket, std::shared_ptr<ConnectionManager> connectionManager);

    void start();

    void stop();

private:
    void do_read_client_request();   // client -> proxy
    void lookup_remote_host();       // proxy -> DNS
    void connect_to_remote_server(asio::ip::tcp::resolver::iterator result);
    void do_write_client_request();  // proxy -> server
    void do_read_server_response();  // server -> proxy
    void do_write_server_response(); // proxy -> client

    asio::ip::tcp::socket socket_;
    asio::ip::tcp::socket remoteSocket_;

    std::shared_ptr<ConnectionManager> connectionManager_;

    struct Payload {
        BufferPtr buffer;
        size_t size;
    };

    std::mutex outboxMutex_;
    std::queue<Payload> outbox_;

    std::mutex serverToClientOutboxMutex_;
    std::queue<Payload> serverToClientOutbox_;

    HttpMessageParser requestParser;
    HttpMessage request;
};

#endif // CONNECTION_H
