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

#include "core/Proxy.h"

#include <atomic>
#include <memory>

#include "core/Transaction.h"

namespace ama {

Proxy::Proxy(const int port, QObject* parent)
    : QObject(parent)
    , port_(port)
    , server_(new Server(port, this))
    , next_id_(1)
{
}

void Proxy::init()
{
    connect(server_, &Server::connection_established, this, &Proxy::on_client_connected);
}

void Proxy::deinit()
{
    disconnect(server_, &Server::connection_established, this, &Proxy::on_client_connected);
}

int Proxy::port() const
{
    return port_;
}

void Proxy::on_client_connected(const std::shared_ptr<Conn>& conn)
{
    auto tx = new Transaction(next_id_++, server_->connection_pool(), conn, this);
    emit transactionStarted(tx);
    tx->begin();
}

} // ama
