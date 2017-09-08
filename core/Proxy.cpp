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

#include <memory>

#include <QDebug>

using namespace ama;

class Proxy::ProxyImpl : public std::enable_shared_from_this<Proxy>,
                         public ConnectionManagerListener,
                         public ConnectionListener
{
public:
    ProxyImpl(const int port);

    void init();
    void deinit();

    virtual void on_connected(const std::shared_ptr<Connection> &connetion) override;

    virtual void client_request_received(const std::shared_ptr<Connection> connection, const HttpMessage &request) override;
    virtual void server_response_received(const std::shared_ptr<Connection> connection, const HttpMessage &request) override;
    virtual void on_error(const std::shared_ptr<Connection> connection, const std::error_code &error) override;
    virtual void connection_closing(const std::shared_ptr<Connection> connection) override;

private:
    int port_;
    Server server_;
};

Proxy::ProxyImpl::ProxyImpl(const int port) :
    ConnectionManagerListener(),
    ConnectionListener(),
    port_(port),
    server_(port)
{
}

void Proxy::ProxyImpl::init()
{
    server_.connection_manager()->add_listener(shared_from_this());
}

void Proxy::ProxyImpl::deinit()
{
    server_.connection_manager()->remove_listener(shared_from_this());
}

void Proxy::ProxyImpl::on_connected(const std::shared_ptr<Connection> &connetion)
{
    // TODO
}

void Proxy::ProxyImpl::client_request_received(const std::shared_ptr<Connection> connection, const HttpMessage &request)
{

}

void Proxy::ProxyImpl::server_response_received(const std::shared_ptr<Connection> connection, const HttpMessage &request)
{

}

void Proxy::ProxyImpl::on_error(const std::shared_ptr<Connection> connection, const std::error_code &error)
{

}

void Proxy::ProxyImpl::connection_closing(const std::shared_ptr<Connection> connection)
{

}


Proxy::Proxy(const int port) :
    impl_(std::make_unique<Proxy::ProxyImpl>(port))
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

void Proxy::on_connected(const std::shared_ptr<Connection> &connection)
{
    connection->add_listener(shared_from_this());
    emit connectionEstablished(connection);
}

void Proxy::client_request_received(const std::shared_ptr<Connection> connection, const HttpMessage &request)
{
    emit requestReceived(connection, request);
}

void Proxy::server_response_received(const std::shared_ptr<Connection> connection, const HttpMessage &response)
{
    emit responseReceived(connection, response);
}

void Proxy::on_error(const std::shared_ptr<Connection> connection, const std::error_code &error)
{
    Q_UNUSED(connection);
    Q_UNUSED(error);
}

void Proxy::connection_closing(const std::shared_ptr<Connection> connection)
{
    connection->remove_listener(shared_from_this());
    emit connectionClosed(connection);
}

int Proxy::port() const
{
    return port_;
}
