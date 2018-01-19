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
#include <os/log.h>

// cpp stuff
#include <iostream>
#include <memory>
#include <string>

// our stuff
#include "Server.h"
#include "TrustyCommon.h"
#include "TrustyService.h"

#include "Log.h"
#include "StringStreamLogValueVisitor.h"

// I wish ASIO would have worked out, but it just couldn't
// seem to handle UNIX sockets on macOS.  Pity, because
// writing socket code the old way works, but is endlessly
// tedious.

using namespace ama::log;
using namespace ama::trusty;

// We're not linking in the log library, so here's a barebones implementation.
namespace ama { namespace log {

void register_log_writer(std::shared_ptr<ILogWriter> &&writer)
{
    // no-op
}

bool is_enabled_for_severity(Severity severity)
{
    return severity >= Severity::Info;
}

void do_log_event(Severity severity, const char *message, const ILogValue &structuredData)
{
    os_log_type_t log_type;
    switch (severity)
    {
    case Severity::Verbose: log_type = OS_LOG_TYPE_DEBUG; break;
    case Severity::Debug:   log_type = OS_LOG_TYPE_INFO; break;
    case Severity::Info:    log_type = OS_LOG_TYPE_DEFAULT; break;
    case Severity::Warn:    log_type = OS_LOG_TYPE_ERROR; break;
    case Severity::Error:   log_type = OS_LOG_TYPE_ERROR; break;
    case Severity::Fatal:   log_type = OS_LOG_TYPE_FAULT; break;
    default:
        std::cerr << "Unexpected log severity: " << static_cast<uint8_t>(severity) << std::endl;
        log_type = OS_LOG_TYPE_DEFAULT;
        break;
    }

    if (os_log_type_enabled(OS_LOG_DEFAULT, log_type))
    {
        StringStreamLogValueVisitor visitor;
        structuredData.accept(visitor);

        os_log_with_type(OS_LOG_DEFAULT, log_type, "%{public}s %{public}s", message, visitor.str().c_str());
    }
}

}} // ama::log

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

    int fd;
    std::error_code ec = lookup_socket_endpoint(&fd);
    if (ec)
    {
        log_event(Severity::Fatal, "Failed to open launchd socket list", IntValue("ec", ec.value()));
        return -1;
    }

    TrustyService service;
    Server server(&service, fd);
    try
    {
        server.serve();
    }
    catch (std::exception &ex)
    {
        log_event(Severity::Fatal, "failed, somehow", CStrValue("ex", ex.what()));
    }

    log_event(Severity::Info, "Hanging up now!");

    return 0;
}
