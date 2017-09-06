#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#pragma once

#include <functional>
#include <string>
#include <system_error>

#include "common.h"
#include "global.h"

#include <asio.hpp>

namespace ama
{

class ConnectionPool;

class Connection
{
public:
    virtual ~Connection() {}

    virtual time_point expires_at() const = 0;
    virtual void set_expires_at(const time_point &tp) = 0;
};

class ConnectionPool
{
public:
    ConnectionPool(asio::io_service &service);

    Connection* make_connection(asio::basic_stream_socket<asio::ip::tcp, asio::stream_socket_service<asio::ip::tcp>> &&socket);

    /**
     * @brief Find any open (and unused) connection to the given endpoint.
     * @param host the remote endpoint's hostname
     * @param port the remote enpoint's TCP port
     * @return Returns a pointer to an open Conn, or @code nullptr if none exists.
     */
    Connection* find_open_connection(const std::string &host, int port);

    void try_open(const std::string *host, int port, std::function<void(Connection*, std::error_code)> callback);

private:
    class impl;
    const std::unique_ptr<impl> impl_;
};

} // namespace ama

#endif // CONNECTIONPOOL_H
