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

#pragma once

#include "global.h"

#include <QObject>

#include <memory>

#include "Connection.h"
#include "ConnectionManager.h"
#include "Server.h"
#include "Transaction.h"

class HttpMessage;

class A_EXPORT Proxy : public QObject,
                       public std::enable_shared_from_this<Proxy>,
                       public ConnectionManagerListener,
                       public ConnectionListener
{
    Q_OBJECT

public:
    Proxy(const int port = 9999);
    virtual ~Proxy();

    int port() const;

    void init();
    void deinit();

    virtual void on_connected(const std::shared_ptr<Connection> &connetion) override;

    virtual void client_request_received(const std::shared_ptr<Connection> connection, const HttpMessage &request) override;
    virtual void server_response_received(const std::shared_ptr<Connection> connection, const HttpMessage &request) override;
    virtual void on_error(const std::shared_ptr<Connection> connection, const std::error_code &error) override;
    virtual void connection_closing(const std::shared_ptr<Connection> connection) override;

signals:
    void connectionEstablished(const std::shared_ptr<Connection> &connection);

    void transactionStarted(std::shared_ptr<Transaction> tx);

    void requestReceived(const std::shared_ptr<Connection> &connection, const HttpMessage &message);
    void responseReceived(const std::shared_ptr<Connection> &connection, const HttpMessage &message);

    void connectionClosed(const std::shared_ptr<Connection> &connection);

private:
    class ProxyImpl;

    std::unique_ptr<ProxyImpl> impl_;

    int port_;
    Server server_;
};

#endif // PROXY_H
