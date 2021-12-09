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

#include "core/common.h"
#include "core/global.h"

#include <QObject>

#include <atomic>
#include <memory>

#include "core/ConnectionPool.h"
#include "core/Server.h"
#include "core/Transaction.h"

namespace ama
{

class HttpMessage;

class A_EXPORT Proxy : public QObject
{
    Q_OBJECT

public:
    Proxy(const int port = 9999, QObject* parent = nullptr);
    virtual ~Proxy() = default;

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
    void transactionStarted(ama::Transaction* tx);

private slots:
    void on_client_connected(const std::shared_ptr<Conn>& conn);

private:
    int port_;
    Server* server_;
    std::atomic_int next_id_;
};

} // namespace ama
