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

// Apple stuff
#include <launch.h>
#include <xpc/xpc.h>

#include "OSLoggable.h"
#include "TrustyServer.h"

int lookup_socket_endpoint(std::error_code &ec)
{
    int result = 0;

    int *fds = nullptr;
    size_t num_sockets;
    int err = launch_activate_socket("com.bendb.amanuensis.Trusty", &fds, &num_sockets);
    if (err == 0)
    {
        result = fds[0];
    }
    else
    {
        ec.assign(err, std::system_category());
    }

    free(fds);

    return result;
}

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    Log log;

    std::error_code ec;
    int fd = lookup_socket_endpoint(ec);
    if (ec)
    {
        log.error() << "Failed to open launchd socket list (" << ec.value() << " " << ec.message() << ")";
        return -1;
    }

    TrustyServer server(fd);
    server.run();


    return 0;
}
