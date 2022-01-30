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
#include <type_traits>

namespace ama {

class ConnectionPool;

namespace details {

using tcp_socket = asio::ip::tcp::socket;
using ssl_socket = asio::ssl::stream<tcp_socket>;

template <typename ...Ts>
inline constexpr bool always_false_v = false;

template <typename Socket>
void close_asio_socket(Socket& socket, std::error_code& ec)
{
    if constexpr(std::is_same_v<Socket, tcp_socket>)
    {
        socket.close(ec);
    }
    else if constexpr(std::is_same_v<Socket, ssl_socket>)
    {
        socket.shutdown(ec);
    }
    else
    {
        static_assert(always_false_v<Socket>);
    }
}

template <typename Socket>
class BaseConnection : public IConnection, public std::enable_shared_from_this<BaseConnection<Socket>>
{
public:
    explicit BaseConnection(Socket&& socket)
        : socket_(std::move(socket))
    {}

    virtual ~BaseConnection() noexcept = default;

    void async_write(const QByteArrayView data, Callback&& callback) override
    {
        auto buf = asio::buffer(data.data(), data.size());
        asio::async_write(socket_, buf, std::move(callback));
    }

    void async_read(QByteArrayView buffer, Callback&& callback) override
    {
        asio::mutable_buffer mb{const_cast<char*>(buffer.data()), static_cast<std::size_t>(buffer.size())};
        asio::async_read(socket_, std::move(mb), asio::transfer_at_least(1), std::move(callback));
    }

    void close(std::error_code& ec) override
    {
        ec = {};

        if (open_.exchange(false))
        {
            close_asio_socket(socket_, ec);
        }
    }

private:
    std::atomic_bool open_;
    Socket socket_;

    friend class ::ama::ConnectionPool;
};

} // details

using TcpConnection = details::BaseConnection<details::tcp_socket>;
using SslConnection = details::BaseConnection<details::ssl_socket>;

using NonConnection = details::BaseConnection<int>;

} // ama
