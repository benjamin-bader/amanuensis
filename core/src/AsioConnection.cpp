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

#include "AsioConnection.h"

namespace ama {

using socket = asio::ip::tcp::socket;
using tls_socket = asio::ssl::stream<socket>;

AsioConnection::AsioConnection(socket&& socket)
    : ssl_{false}
    , socket_{std::move(socket)}
{
}

AsioConnection::AsioConnection(tls_socket&& socket)
    : ssl_{true}
    , sslStream_(std::move(socket))
{
}

AsioConnection::~AsioConnection()
{
    if (ssl_)
    {
        sslStream_.~stream();
    }
    else
    {
        socket_.~basic_stream_socket();
    }
}

void AsioConnection::async_read(QByteArrayView buffer, Callback&& callback)
{
    asio::mutable_buffer mb{const_cast<char*>(buffer.data()), static_cast<std::size_t>(buffer.size())};
    if (ssl_)
    {
        asio::async_read(sslStream_, std::move(mb), asio::transfer_at_least(1), std::move(callback));
    }
    else
    {
        asio::async_read(socket_, std::move(mb), asio::transfer_at_least(1), std::move(callback));
    }
}

void AsioConnection::async_write(const QByteArrayView data, Callback&& callback)
{
    auto buf = asio::buffer(data.data(), data.size());
    if (ssl_)
    {
        asio::async_write(sslStream_, buf, std::move(callback));
    }
    else
    {
        asio::async_write(socket_, buf, std::move(callback));
    }
}

void AsioConnection::close(std::error_code& ec)
{
    ec = {};

    if (ssl_)
    {
        sslStream_.shutdown(ec);
    }
    else
    {
        socket_.close(ec);
    }
}

} // ama
