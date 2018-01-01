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
#include <poll.h>
#include <string.h>     // for strerror
#include <sys/socket.h>

#include <system_error>

#include "ClientConnection.h"
#include "TLog.h"

namespace ama { namespace trusty {

Server::Server(IService *service, int server_fd)
    : service_(service)
    , server_fd_(server_fd)
    , accept_timeout_(std::chrono::seconds(60))
{
    // no-op
}

void Server::serve()
{
    int client_fd;
    while ((client_fd = accept_next_client()) >= 0)
    {
        handle_client_session(client_fd);
    }
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
        log_error("poll() failed: errno=%d (%s)", errno, ::strerror(errno));
        return -2;
    }

    if (ready_count == 0)
    {
        log_error("poll(): no connection?");
        return -1;
    }

    int connection_fd = ::accept(server_fd_, addr, &size);
    if (connection_fd < 0)
    {
        log_error("accept() failed: errno=%d (%s)", errno, ::strerror(errno));
        return -2;
    }

    return connection_fd;
}

void Server::handle_client_session(int client_fd)
{
    try
    {
        ClientConnection conn(service_, client_fd);
        conn.handle();
    }
    catch (const std::exception &ex)
    {
        log_critical("Exception in handle_client_session(): %s", ex.what());
    }
}

}} // ama::trusty
