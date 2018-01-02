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

#ifndef LOGGING_H
#define LOGGING_H

#pragma once

#include <initializer_list>
#include <memory>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include "global.h"

namespace ama {

namespace LogSinks {

spdlog::sink_ptr A_EXPORT syslog_sink();
spdlog::sink_ptr A_EXPORT stderr_sink();
spdlog::sink_ptr A_EXPORT windows_debug_sink();
spdlog::sink_ptr A_EXPORT mac_os_log_sink();

} // namespace LogSinks

void A_EXPORT set_default_sinks(std::initializer_list<spdlog::sink_ptr> default_sinks);

std::shared_ptr<spdlog::logger> A_EXPORT get_logger(const std::string& name);

} // namespace ama

#endif // LOGGING_H
