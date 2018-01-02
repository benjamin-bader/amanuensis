// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2018 Benjamin Bader
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

#ifndef TLOG_H
#define TLOG_H

#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include "OsLogSink.h"

namespace ama { namespace trusty {

namespace details {

extern std::once_flag g_logger_init_flag;
extern std::shared_ptr<spdlog::logger> g_logger;

}

void init_logging(const std::string& logger_name);


template <typename... Args>
void log_trace(const char* fmt, const Args&... args)
{
    details::g_logger->trace(fmt, args...);
}

template <typename... Args>
void log_debug(const char* fmt, const Args& ...args)
{
    details::g_logger->debug(fmt, args...);
}

template <typename... Args>
void log_info(const char* fmt, const Args& ...args)
{
    details::g_logger->info(fmt, args...);
}

template <typename... Args>
void log_warn(const char* fmt, const Args& ...args)
{
    details::g_logger->warn(fmt, args...);
}

template <typename... Args>
void log_error(const char* fmt, const Args& ...args)
{
    details::g_logger->error(fmt, args...);
}

template <typename... Args>
void log_critical(const char* fmt, const Args& ...args)
{
    details::g_logger->critical(fmt, args...);
}

}} // namespace ama::trusty

#endif // TLOG_H
