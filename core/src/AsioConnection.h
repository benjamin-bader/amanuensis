// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2021 Benjamin Bader
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

#pragma once

#include "core/IConnection.h"

#include <asio.hpp>
#include <asio/ssl.hpp>

#include <atomic>
#include <memory>

namespace ama {

class IAsioConnection : public IConnection, public std::enable_shared_from_this<IAsioConnection>
{
public:
    virtual ~IAsioConnection() noexcept = default;

    virtual void async_write(const QByteArrayView data, Callback&& callback) override = 0;
    virtual void async_read(QByteArrayView buffer, Callback&&) override = 0;
    virtual void close(std::error_code& ec) override = 0;
};

class AsioConnection : public IAsioConnection
{
public:
    explicit AsioConnection(asio::ip::tcp::socket&& socket);
    virtual ~AsioConnection();

    void async_write(const QByteArrayView data, Callback&& callback) override;
    void async_read(QByteArrayView buffer, Callback&&) override;
    void close(std::error_code& ec) override;

private:
    std::atomic_bool open_;
    asio::ip::tcp::socket socket_;

    friend class ConnectionPool;
};

class TlsAsioConnection : public IAsioConnection
{
public:
    explicit TlsAsioConnection(asio::ssl::stream<asio::ip::tcp::socket>&& socket);
    virtual ~TlsAsioConnection();

    void async_write(const QByteArrayView data, Callback&& callback) override;
    void async_read(QByteArrayView buffer, Callback&&) override;
    void close(std::error_code& ec) override;

private:
    std::atomic_bool open_;
    asio::ssl::stream<asio::ip::tcp::socket> sslStream_;

    friend class ConnectionPool;
};

} // ama
