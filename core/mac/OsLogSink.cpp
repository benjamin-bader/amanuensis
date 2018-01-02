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

#include "OsLogSink.h"

namespace ama {

OsLogSink::OsLogSink() : OsLogSink(OS_LOG_DEFAULT)
{
}

OsLogSink::OsLogSink(os_log_t log) : log_(log)
{
    log_types_by_level_[spdlog::level::trace] = OS_LOG_TYPE_DEBUG;
    log_types_by_level_[spdlog::level::debug] = OS_LOG_TYPE_INFO;
    log_types_by_level_[spdlog::level::info] = OS_LOG_TYPE_DEFAULT;
    log_types_by_level_[spdlog::level::warn] = OS_LOG_TYPE_DEFAULT;
    log_types_by_level_[spdlog::level::err] = OS_LOG_TYPE_ERROR;
    log_types_by_level_[spdlog::level::critical] = OS_LOG_TYPE_FAULT;
    log_types_by_level_[spdlog::level::off] = OS_LOG_TYPE_DEBUG;
}

void OsLogSink::_sink_it(const spdlog::details::log_msg &msg)
{
    const char* formatted = msg.formatted.c_str();
    os_log_with_type(log_, log_type_for_level(msg.level), "%{public}s", formatted);
}

void OsLogSink::_flush()
{
    // no-op
}

os_log_type_t OsLogSink::log_type_for_level(const spdlog::level::level_enum level) const
{
    return log_types_by_level_[static_cast<int>(level)];
}

} // namespace ama::trusty
