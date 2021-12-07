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

#include "TLog.h"

#include <iostream>
#include <string>

#include "spdlog/sinks/stdout_sinks.h"

namespace ama { namespace trusty {

namespace details {

std::once_flag g_logger_init_flag;

std::shared_ptr<spdlog::logger> g_logger;

} // details

void init_logging(const std::string& logger_name)
{
    std::call_once(details::g_logger_init_flag, [&logger_name]() {
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<ama::trusty::OsLogSink>());
        sinks.push_back(std::make_shared<spdlog::sinks::stderr_sink_mt>());

        spdlog::set_error_handler([](const std::string& err_msg)
        {
            std::cerr << "FATAL ERROR FROM SPDLOG: " << err_msg << std::endl;
        });

        details::g_logger = std::make_shared<spdlog::logger>(logger_name, sinks.begin(), sinks.end());

        spdlog::register_logger(details::g_logger);
    });
}

}} // namespace ama::trusty
