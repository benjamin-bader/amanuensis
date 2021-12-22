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

#include "constants.h"
#include "TrustyService.h"
#include "XpcServer.h"

#include "log/Log.h"
#include "log/OsLogWriter.h"

#include <memory>

using namespace ama;
using namespace ama::trusty;

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    log::register_log_writer(std::make_shared<log::OsLogWriter>());

    std::shared_ptr<IService> service = std::make_shared<TrustyService>();

    auto xpcServer = std::make_shared<XpcServer>(service, MACH_SERVICE_NAME);

    xpcServer->serve();

    log::info("Hanging up now!");

    return 0;
}
