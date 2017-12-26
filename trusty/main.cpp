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

/**
 * This file contains the trusted helper ("Trusty") for Amanuensis,
 * which handles
 */

// Apple stuff
#include <launch.h>

// UNIX stuff
#include <syslog.h>

// cpp stuff
#include <iostream>
#include <memory>
#include <string>

// our stuff
#include "Server.h"
#include "TrustyCommon.h"
#include "TrustyService.h"

// I wish ASIO would have worked out, but it just couldn't
// seem to handle UNIX sockets on macOS.  Pity, because
// writing socket code the old way works, but is endlessly
// tedious.

std::error_code lookup_socket_endpoint(int *fd)
{
    std::error_code ec;

    if (fd == nullptr)
    {
        return std::make_error_code(std::errc::invalid_argument);
    }

    *fd = -1;

    int *fds = nullptr;
    size_t num_sockets;
    int err = launch_activate_socket(ama::kPlistLaunchdSocketName, &fds, &num_sockets);
    if (err == 0 && num_sockets > 0)
    {
        *fd = fds[0];
    }
    else
    {
        ec.assign(err, std::system_category());
    }

    free(fds);

    return ec;
}

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    std::cerr << "Our amazing journey begins" << std::endl;

    int fd;
    std::error_code ec = lookup_socket_endpoint(&fd);
    if (ec)
    {
        syslog(LOG_INFO, "Failed to open launchd socket list: %d", ec.value());
        return -1;
    }

    ama::trusty::TrustyService service;
    ama::trusty::Server server(&service, fd);
    try
    {
        server.serve();
    }
    catch (std::exception &ex)
    {
        std::cerr << "Failed, somehow: " << ex.what() << std::endl;
    }

    std::cerr << "Hanging up now!" << std::endl;

    return 0;
}
