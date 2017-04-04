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

#ifndef PROXY_H
#define PROXY_H

#include "global.h"

#include <QObject>

#include <memory>

#include <asio.hpp>

class A_EXPORT Proxy : public QObject
{
    Q_OBJECT

public:
    Proxy(const int port = 9999);
    virtual ~Proxy();

    const int port() const;

signals:
    void* requestCreated();

private:
    void do_accept();

private:
    int port_;
    asio::io_service io_service_;
    asio::signal_set signals_;
    asio::ip::tcp::acceptor acceptor_;
    asio::ip::tcp::socket socket_;
};

#endif // PROXY_H
