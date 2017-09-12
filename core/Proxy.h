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

#include "common.h"
#include "global.h"

#include <QObject>

#include <memory>

#include "ConnectionPool.h"
#include "Server.h"
#include "Transaction.h"

namespace ama
{

class HttpMessage;

class A_EXPORT Proxy : public QObject,
                       public std::enable_shared_from_this<Proxy>
{
    Q_OBJECT

public:
    Proxy(const int port = 9999);
    virtual ~Proxy();

    int port() const;

    void init();
    void deinit();

signals:
    /**
     * @brief Emitted when a client transaction is about to begin.
     *
     * Subscribers should note that this signal is always emitted on a
     * background thread.  They should not do anything _except_ register
     * themselves as a listener on the new transaction!
     *
     * Connections _should not_ be QUEUED; when this method completes,
     * the transaction will be started and any queued listeners may
     * miss transaction events.
     *
     * @param tx the new transaction.
     */
    void transactionStarted(std::shared_ptr<Transaction> tx);

private:
    class ProxyImpl;

    std::unique_ptr<ProxyImpl> impl_;

    int port_;
    Server server_;
};

} // namespace ama

#endif // PROXY_H
