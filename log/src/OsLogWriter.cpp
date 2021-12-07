// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2021 Benjamin Bader
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

#include "log/OsLogWriter.h"

#include "log/StringStreamLogValueVisitor.h"

namespace ama::log {

void OsLogWriter::write(Severity severity, const char* message, const ILogValue& value)
{
    auto type = log_type_for_severity(severity);
    if (os_log_type_enabled(OS_LOG_DEFAULT, type))
    {
        StringStreamLogValueVisitor visitor;
        value.accept(visitor);

        os_log_with_type(OS_LOG_DEFAULT, type, "%{public}s %{public}s", message, visitor.str().c_str());
    }
}

os_log_type_t OsLogWriter::log_type_for_severity(Severity severity)
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

} // ama::log
