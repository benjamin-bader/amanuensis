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

#include <array>
#include <memory>

#include <QObject>

#include <asio/ip/tcp.hpp>

class Connection :
        public QObject,
        public std::enable_shared_from_this<Connection>
{
    Q_OBJECT

public:
    explicit Connection(asio::ip::tcp::socket socket);

    void start();

    void stop();

private:
    asio::ip::tcp::socket socket_;

    std::array<char, 8192> buffer_;
};

#endif // CONNECTION_H
