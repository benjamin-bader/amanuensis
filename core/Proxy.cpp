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
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <QDebug>
#include <QThread>

#include <asio.hpp>

Proxy::Proxy(const int port) :
    port_(port),
    io_service_(),
    signals_(io_service_),
    acceptor_(io_service_),
    socket_(io_service_),
    acceptorThread_([this]{ while (true) { io_service_.run(); } })
{
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
#if defined(SIGQUIT)
    signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)

    signals_.async_wait([this](std::error_code /*ec*/, int /*signo*/) {
       acceptor_.close();
    });

    asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    do_accept();
}

Proxy::~Proxy()
{
    acceptor_.close();
    signals_.clear();
    io_service_.stop();
}

const int Proxy::port() const
{
    return port_;
}

static asio::const_buffer reply = asio::buffer("HELLO THERE \r\n");

void Proxy::do_accept()
{
    // Enable the acceptor to receive IPv4 traffic as well as IPv6;
    // v4 addresses will be presented as v6 addresses using a well-known
    // mapping (https://en.wikipedia.org/wiki/IPv6#IPv4-mapped_IPv6_addresses)
    //
    // Maybe it's better to use two sockets?  This precludes Windows XP and prior,
    // as well as OpenBSD.
    //socket_.set_option(asio::ip::v6_only(false));

    acceptor_.async_accept(socket_, [this](asio::error_code ec) {
        qDebug() << "HI I HAVE ACCEPTED UR SOCKET";

        if (!acceptor_.is_open())
        {
            qFatal("BOOM");
            return;
        }

        if (!ec)
        {
            std::vector<asio::const_buffer> buffers;
            buffers.push_back(reply);

            socket_.async_write_some(buffers, [this](std::error_code ec2, std::size_t bytesWritten) {
               qDebug() << "Sent " << bytesWritten << " bytes";
               socket_.close();
            });

            do_accept();
        }
        else
        {
            qDebug() << "Sad day: " << QString(ec.message().c_str());
        }
    });
}
