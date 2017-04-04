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

#include <csignal>
#include <utility>

#include <asio.hpp>

Proxy::Proxy(const int port) :
    port_(port),
    io_service_(),
    signals_(io_service_),
    acceptor_(io_service_),
    socket_(io_service_)
{
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
#if defined(SIGQUIT)
    signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)

    signals_.async_wait([this](std::error_code /*ec*/, int /*signo*/) {
       acceptor_.close();
    });

    asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string("::1"), port);

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();



    do_accept();


}

Proxy::~Proxy()
{
    // nothing
}

const int Proxy::port() const
{
    return port_;
}

void Proxy::do_accept()
{
    asio::ip::tcp::socket socket(io_service_);

    // Enable the acceptor to receive IPv4 traffic as well as IPv6;
    // v4 addresses will be presented as v6 addresses using a well-known
    // mapping (https://en.wikipedia.org/wiki/IPv6#IPv4-mapped_IPv6_addresses)
    //
    // Maybe it's better to use two sockets?  This precludes Windows XP and prior,
    // as well as OpenBSD.
    socket.set_option(asio::ip::v6_only(false));

    acceptor_.async_accept(socket, [this, &socket](asio::error_code ec) {
        if (!acceptor_.is_open())
        {
            return;
        }

        if (!ec)
        {
            std::move(socket);
            // we've got a connection!  do something with socket_.
        }

        do_accept();
    });
}
