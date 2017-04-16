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

#pragma once

#include <memory>
#include <system_error>

#include "asiofwd.h"
#include "HttpMessage.h"
#include "Listenable.h"

class Connection;
class ConnectionManager;

class ConnectionListener
{
public:
    ConnectionListener() {}
    virtual ~ConnectionListener() {}

    virtual void client_request_received(const std::shared_ptr<Connection> connection, const HttpMessage &request) = 0;
    virtual void server_response_received(const std::shared_ptr<Connection> connection, const HttpMessage &response) = 0;

    virtual void on_error(const std::shared_ptr<Connection> connection, const std::error_code &error) = 0;

    virtual void connection_closing(const std::shared_ptr<Connection> connection) = 0;
};

class Connection : public std::enable_shared_from_this<Connection>,
                   public Listenable<ConnectionListener>
{
public:
    Connection() = delete;
    Connection& operator=(const Connection&) = delete;
    virtual ~Connection();

    explicit Connection(asio::basic_stream_socket<asio::ip::tcp, asio::stream_socket_service<asio::ip::tcp>> socket, std::shared_ptr<ConnectionManager> connectionManager);

    void start();
    void stop();

    int id() const;
    void set_id(int id);

private:
    class impl;
    const std::unique_ptr<impl> impl_;

    void do_read_client_request();   // client -> proxy
    void lookup_remote_host();       // proxy -> DNS
    void do_write_client_request();  // proxy -> server
    void do_read_server_response();  // server -> proxy
    void do_write_server_response(); // proxy -> client

    void notify_client_request_received();
    void notify_server_response_received();
    void notify_error(const std::error_code &error);
    void notify_connection_closing();
};

#endif // CONNECTION_H
