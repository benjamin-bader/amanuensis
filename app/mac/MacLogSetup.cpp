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

#include <string>

#include <os/log.h>

#include <QApplication>
#include <spdlog/spdlog.h>

#include "Log.h"
#include "Logging.h"
#include "StringStreamLogValueVisitor.h"
#include "TLog.h"

namespace ama {

class OsLogWriter : public log::ILogWriter
{
public:
    void write(log::Severity severity, const char* msg, const log::ILogValue& value);

private:
    static os_log_type_t log_type_for_severity(log::Severity severity);
};

os_log_type_t OsLogWriter::log_type_for_severity(log::Severity severity)
{
    switch (severity)
    {
    case log::Severity::Verbose: return OS_LOG_TYPE_DEBUG;
    case log::Severity::Debug: return OS_LOG_TYPE_INFO;
    case log::Severity::Info: return OS_LOG_TYPE_DEFAULT;
    case log::Severity::Warn: return OS_LOG_TYPE_DEFAULT;
    case log::Severity::Error: return OS_LOG_TYPE_ERROR;
    case log::Severity::Fatal: return OS_LOG_TYPE_FAULT;
    default:
        return OS_LOG_TYPE_DEFAULT;
    }
}

void OsLogWriter::write(log::Severity severity, const char *msg, const log::ILogValue &value)
{
    os_log_type_t log_type = log_type_for_severity(severity);
    if (os_log_type_enabled(OS_LOG_DEFAULT, log_type))
    {
        log::StringStreamLogValueVisitor visitor;
        value.accept(visitor);

        os_log_with_type(OS_LOG_DEFAULT, log_type, "%{public}s %{public}s", msg, visitor.str().c_str());
    }
}

void MacLogSetup::configure_logging()
{
    std::string app_name = QCoreApplication::applicationName().toStdString();
    ama::trusty::init_logging(app_name);

    set_default_sinks({
        LogSinks::stderr_sink(),
        LogSinks::mac_os_log_sink()
    });

    log::register_log_writer(std::make_shared<OsLogWriter>());
}

}
