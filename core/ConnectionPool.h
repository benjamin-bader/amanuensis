#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <system_error>

#include "common.h"
#include "global.h"

#include "Listenable.h"

#include <asio.hpp>

namespace ama
{

class ConnectionPool;

class Conn : public std::enable_shared_from_this<Conn>
{
public:
    Conn(asio::io_service &service);
    Conn(asio::ip::tcp::socket &&socket);

    ~Conn();

    time_point expires_at() const
    {
        return expires_at_;
    }

    void set_expires_at(const time_point &tp)
    {
        expires_at_ = tp;
    }

    /**
     * @brief Mark this connection for closure when it is returned
     *        to the connection pool.
     */
    void force_close_on_return()
    {
        should_close_ = true;
    }

    template <typename BufferSequence, typename WriteHandler>
    void async_write(const BufferSequence &bufferSequence, WriteHandler&& handler)
    {
        asio::async_write(socket_, bufferSequence, std::move(handler));
    }

    template <typename MutableBufferSequence, typename ReadHandler>
    void async_read_some(MutableBufferSequence &buffers, ReadHandler &&handler)
    {
        socket_.async_read_some(asio::buffer(buffers), std::move(handler));
    }

private:
    asio::ip::tcp::socket socket_;
    time_point expires_at_;
    bool should_close_;

    friend class ConnectionPool;
};



class A_EXPORT ConnectionPoolListener
{
public:
    virtual void on_client_connected(std::shared_ptr<Conn> connection) = 0;
};

class ConnectionPool : public Listenable<ConnectionPoolListener>
{
public:
    ConnectionPool(asio::io_service &service);
    ~ConnectionPool();

    std::shared_ptr<Conn> make_connection(asio::ip::tcp::socket &&socket);

    /**
     * @brief Find any open (and unused) connection to the given endpoint.
     * @param host the remote endpoint's hostname
     * @param port the remote enpoint's TCP port
     * @return Returns a pointer to an open Conn, or @code nullptr if none exists.
     */
    std::shared_ptr<Conn> find_open_connection(const std::string &host, int port);

    void try_open(const std::string &host, const std::string &port, std::function<void(std::shared_ptr<Conn>, std::error_code)> &&callback);

private:
    class impl;
    const std::shared_ptr<impl> impl_;
};

} // namespace ama

#endif // CONNECTIONPOOL_H
