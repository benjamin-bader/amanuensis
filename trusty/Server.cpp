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

#include "Server.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <system_error>

#include "MessageProcessor.h"
#include "Service.h" // for IService

using namespace ama::trusty;

Server::Server(IService *service, int server_fd)
    : service_(service)
    , server_fd_(server_fd)
    , accept_timeout_(std::chrono::seconds(10))
{
    // no-op
}

void Server::serve()
{
    int client_fd;
    while ((client_fd = accept_next_client()) >= 0)
    {
        validate_client(client_fd);
        handle_client_session(client_fd);
    }
}

void Server::validate_client(int client_fd)
{
    uid_t euid;
    uid_t egid;

    if (::getpeereid(client_fd, &euid, &egid) == -1)
    {
        throw std::system_error(errno, std::system_category());
    }

    std::cerr << "euid=" << euid << " egid=" << egid << std::endl;
}

int Server::accept_next_client()
{
    socklen_t size = sizeof(struct sockaddr) + 128;
    char addr_data[size];
    struct sockaddr *addr = (struct sockaddr *) &addr_data;

    struct pollfd fds;
    fds.fd = server_fd_;
    fds.events = POLLIN;

    auto timeout_millis = std::chrono::duration_cast<std::chrono::milliseconds>(accept_timeout_);


    int ready_count = ::poll(&fds, 1, timeout_millis.count());

    if (ready_count == -1)
    {
        // womp womp
        std::cerr << "poll() failed: errno=" << errno << std::endl;
        return -2;
    }

    if (ready_count == 0)
    {
        std::cerr << "poll(): No connection?" << std::endl;
        return -1;
    }

    int connection_fd = ::accept(server_fd_, addr, &size);
    if (connection_fd < 0)
    {
        std::cerr << "accept(): failed; errno=" << errno << std::endl;
        return -2;
    }

    return connection_fd;
}

void Server::handle_client_session(int client_fd)
{
    MessageProcessor processor(client_fd);

    static const Message kAck { MessageType::Ack, {} };

    Message msg;
    do
    {
        msg = processor.recv();

        switch (msg.type)
        {
        case MessageType::SetProxyHost:
        {
            std::string requested_host(msg.payload.begin(), msg.payload.end());
            service_->set_http_proxy_host(requested_host);
            processor.send(kAck);
            break;
        }

        case MessageType::SetProxyPort:
        {
            int port = *((int*) msg.payload.data());
            service_->set_http_proxy_port(port);
            processor.send(kAck);
            break;
        }

        case MessageType::GetProxyHost:
        {
            auto host = service_->get_http_proxy_host();

            Message reply;
            reply.type = MessageType::Ack;
            reply.payload.assign(host.begin(), host.end());

            processor.send(reply);
            break;
        }

        case MessageType::GetProxyPort:
        {
            auto port = service_->get_http_proxy_port();

            uint8_t *result_bytes = (uint8_t *) &port;

            Message reply;
            reply.type = MessageType::Ack;
            reply.payload.assign(result_bytes, result_bytes + sizeof(port));

            processor.send(reply);
            break;
        }

        case MessageType::ClearProxySettings:
        {
            service_->reset_proxy_settings();
            processor.send(kAck);
            break;
        }

        case MessageType::Disconnect:
        {
            // Done!
            break;
        }

        default:
        {
            std::cerr << "ERROR: Received response message-type from a client!  type=" << msg.type << std::endl;
            break;
        }

        }; // switch
    } while (msg.type != MessageType::Disconnect);
}
