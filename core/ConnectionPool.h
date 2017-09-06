#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <system_error>

#include "asiofwd.h"
#include "global.h"

class ConnectionPool;

class Conn
{
public:
    Conn(asio::basic_stream_socket<asio::ip::tcp, asio::stream_socket_service<asio::ip::tcp>> &&socket);

    std::chrono::time_point expires_at() const;
    void expires_at(const std::chrono::time_point &expires_at);



private:
    asio::basic_stream_socket<asio::ip::tcp, asio::stream_socket_service<asio::ip::tcp>> socket_;
};

class ConnectionPool
{
public:
    ConnectionPool(asio::io_service &service);
    ~ConnectionPool();

    Conn make_client_connection(asio::basic_stream_socket<asio::ip::tcp, asio::stream_socket_service<asio::ip::tcp> &&socket);

    /**
     * @brief Find any open (and unused) connection to the given endpoint.
     * @param host the remote endpoint's hostname
     * @param port the remote enpoint's TCP port
     * @return Returns a pointer to an open Conn, or @code nullptr if none exists.
     */
    Conn* find_open(const std::string &host, int port);

    void try_open(const std::string *host, int port, std::function<void(Conn*, std::error_code)> callback);

private:
    class impl;
    const std::unique_ptr<impl> impl_;
}

#endif // CONNECTIONPOOL_H
