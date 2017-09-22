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

#include "Proxy.h"

#include <atomic>
#include <memory>

#include <QDebug>

#include "ProxyTransaction.h"

using namespace ama;

class Proxy::ProxyImpl : public std::enable_shared_from_this<ProxyImpl>,
                         public ConnectionPoolListener
{
public:
    ProxyImpl(const int port, Proxy *proxy);

    void init();
    void deinit();

    int port() const { return port_; }

    virtual void on_client_connected(std::shared_ptr<Conn> connection) override;

private:
    int port_;
    Server server_;

    std::atomic_int next_id_;

    Proxy *proxy_;
};

Proxy::ProxyImpl::ProxyImpl(const int port, Proxy *proxy) :
    std::enable_shared_from_this<ProxyImpl>(),
    ConnectionPoolListener(),
    port_(port),
    server_(port),
    next_id_(1),
    proxy_(proxy)
{
}

void Proxy::ProxyImpl::init()
{
    server_.connection_pool()->add_listener(shared_from_this());
}

void Proxy::ProxyImpl::deinit()
{
    server_.connection_pool()->remove_listener(shared_from_this());
}

void Proxy::ProxyImpl::on_client_connected(std::shared_ptr<Conn> connection)
{
    auto tx = std::make_shared<ProxyTransaction>(next_id_++, server_.connection_pool(), connection);
    emit proxy_->transactionStarted(tx);
    tx->begin();
}


Proxy::Proxy(const int port) :
    impl_(std::make_shared<Proxy::ProxyImpl>(port, this))
{
}

Proxy::~Proxy()
{
}

void Proxy::init()
{
    impl_->init();

}

void Proxy::deinit()
{
    impl_->deinit();
}

int Proxy::port() const
{
    return impl_->port();
}
