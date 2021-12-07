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

#include "MacLogSetup.h"

#include <memory>
#include <string>

#include <QApplication>
#include <spdlog/spdlog.h>

#include "core/Logging.h"
#include "log/Log.h"
#include "log/OsLogWriter.h"
#include "trusty/TLog.h"

namespace ama {

void MacLogSetup::configure_logging()
{
    std::string app_name = QCoreApplication::applicationName().toStdString();
    ama::trusty::init_logging(app_name);

    set_default_sinks({
        LogSinks::stderr_sink(),
        LogSinks::mac_os_log_sink()
    });

    log::register_log_writer(std::make_shared<log::OsLogWriter>());

    log::info("OS logger registered", log::IntValue("code", 1337));
}

}
