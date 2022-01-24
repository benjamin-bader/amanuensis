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
    : open_{true}
    , socket_{std::move(socket)}
{
}

AsioConnection::~AsioConnection()
{
}

void AsioConnection::async_read(QByteArrayView buffer, Callback&& callback)
{
    asio::mutable_buffer mb{const_cast<char*>(buffer.data()), static_cast<std::size_t>(buffer.size())};
    asio::async_read(socket_, std::move(mb), asio::transfer_at_least(1), std::move(callback));
}

void AsioConnection::async_write(const QByteArrayView data, Callback&& callback)
{
    auto buf = asio::buffer(data.data(), data.size());
    asio::async_write(socket_, buf, std::move(callback));
}

void AsioConnection::close(std::error_code& ec)
{
    ec = {};

    if (open_.exchange(false))
    {
        socket_.close(ec);
    }
}

TlsAsioConnection::TlsAsioConnection(tls_socket&& socket)
    : open_{true}
    , sslStream_(std::move(socket))
{
}

TlsAsioConnection::~TlsAsioConnection()
{
}

void TlsAsioConnection::async_read(QByteArrayView buffer, Callback&& callback)
{
    asio::mutable_buffer mb{const_cast<char*>(buffer.data()), static_cast<std::size_t>(buffer.size())};
    asio::async_read(sslStream_, std::move(mb), asio::transfer_at_least(1), std::move(callback));
}

void TlsAsioConnection::async_write(const QByteArrayView data, Callback&& callback)
{
    auto buf = asio::buffer(data.data(), data.size());
    asio::async_write(sslStream_, buf, std::move(callback));
}

void TlsAsioConnection::close(std::error_code& ec)
{
    ec = {};

    if (open_.exchange(false))
    {
        sslStream_.shutdown(ec);
    }
}

} // ama
