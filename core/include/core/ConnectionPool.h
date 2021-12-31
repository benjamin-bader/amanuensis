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

#pragma once

#include "core/global.h"
#include "core/IConnection.h"

#include <QObject>

#include <functional>
#include <memory>
#include <string>
#include <system_error>

#include <asio.hpp>

namespace ama
{

class ConnectionPool : public QObject
{
    Q_OBJECT

public:
    using OpenCallback = std::function<void(std::shared_ptr<IConnection>, std::error_code)>;

    ConnectionPool(asio::io_context& context, QObject* parent = nullptr);
    ~ConnectionPool();

    std::shared_ptr<IConnection> make_connection(asio::ip::tcp::socket&& socket);

    /**
     * @brief Find any open (and unused) connection to the given endpoint.
     * @param host the remote endpoint's hostname
     * @param port the remote enpoint's TCP port
     * @return Returns a pointer to an open Conn, or @code nullptr if none exists.
     */
    std::shared_ptr<IConnection> find_open_connection(const std::string& host, int port);

    void try_open(const std::string& host, const std::string& port, OpenCallback&& callback);

signals:
    void client_connected(const std::shared_ptr<IConnection>& connection);

private:
    asio::io_context& context_;
    asio::ip::tcp::resolver resolver_;
};

} // namespace ama
