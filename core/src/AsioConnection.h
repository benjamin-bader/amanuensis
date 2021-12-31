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

#include <memory>

namespace ama {

class AsioConnection : public IConnection, public std::enable_shared_from_this<AsioConnection>
{
public:
    explicit AsioConnection(asio::ip::tcp::socket&& socket);
    explicit AsioConnection(asio::ssl::stream<asio::ip::tcp::socket>&& socket);
    virtual ~AsioConnection();

    void async_write(const QByteArrayView data, Callback&& callback) override;
    void async_read(QByteArrayView buffer, Callback&&) override;
    void close(std::error_code& ec) override;

private:
    bool ssl_;
    union {
        asio::ssl::stream<asio::ip::tcp::socket> sslStream_;
        asio::ip::tcp::socket socket_;
    };

    friend class ConnectionPool;
};

} // ama
