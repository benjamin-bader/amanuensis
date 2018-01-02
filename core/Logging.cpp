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

#include "Logging.h"

#include <algorithm>
#include <mutex>
#include <vector>

#if defined(Q_OS_WIN)
#include "win/OutputDebugStringSink.h"
#elif defined(Q_OS_DARWIN)
#include "OsLogSink.h"
#endif

namespace ama {

namespace LogSinks {

namespace {

std::mutex g_sink_mutex;
std::vector<spdlog::sink_ptr> g_default_sinks;

std::once_flag g_sinks_init;
spdlog::sink_ptr g_syslog_sink;
spdlog::sink_ptr g_stderr_sink;
spdlog::sink_ptr g_windows_debug_sink;
spdlog::sink_ptr g_mac_os_log_sink;

void init_sinks()
{
    std::call_once(g_sinks_init, []() {
#ifdef SPDLOG_ENABLE_SYSLOG
        g_syslog_sink = std::make_shared<spdlog::sinks::syslog_sink>();
#else
        g_syslog_sink = nullptr;
#endif

        g_stderr_sink = std::make_shared<spdlog::sinks::stderr_sink_mt>();

#ifdef Q_OS_WIN
        g_windows_debug_sink = std::make_shared<ama::OutputDebugStringSink>();
#else
        g_windows_debug_sink = nullptr;
#endif

#ifdef Q_OS_DARWIN
        g_mac_os_log_sink = std::make_shared<ama::OsLogSink>();
#else
        g_mac_os_log_sink = nullptr;
#endif
    });
}

}

spdlog::sink_ptr syslog_sink()
{
    init_sinks();
    return g_syslog_sink;
}

spdlog::sink_ptr stderr_sink()
{
    init_sinks();
    return g_stderr_sink;
}

spdlog::sink_ptr windows_debug_sink()
{
    init_sinks();
    return g_windows_debug_sink;
}

spdlog::sink_ptr mac_os_log_sink()
{
    init_sinks();
    return g_mac_os_log_sink;
}

} // namespace LogSinks

void set_default_sinks(std::initializer_list<spdlog::sink_ptr> default_sinks)
{
    std::lock_guard<std::mutex> lock(LogSinks::g_sink_mutex);
    LogSinks::g_default_sinks.assign(default_sinks.begin(), default_sinks.end());

    auto to_erase = std::find_if(
                LogSinks::g_default_sinks.begin(),
                LogSinks::g_default_sinks.end(),
                [](spdlog::sink_ptr ptr)
            {
                return ptr == nullptr;
            });

    if (to_erase != LogSinks::g_default_sinks.end())
    {
        LogSinks::g_default_sinks.erase(to_erase);
    }

    if (LogSinks::g_default_sinks.empty())
    {
        LogSinks::g_default_sinks.push_back(LogSinks::stderr_sink());
    }
}

std::shared_ptr<spdlog::logger> get_logger(const std::string& name)
{
    std::lock_guard<std::mutex> lock(LogSinks::g_sink_mutex);

    auto logger = spdlog::get(name);
    if (! logger)
    {
        if (LogSinks::g_default_sinks.empty())
        {
            LogSinks::g_default_sinks.push_back(LogSinks::stderr_sink());
        }

        logger = std::make_shared<spdlog::logger>(name, LogSinks::g_default_sinks.begin(), LogSinks::g_default_sinks.end());
        spdlog::register_logger(logger);
    }
    return logger;
}

std::shared_ptr<spdlog::logger> get_logger(const std::string& name, std::initializer_list<spdlog::sink_ptr> sinks)
{
    auto logger = spdlog::get(name);
    if (! logger)
    {
        logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
        spdlog::register_logger(logger);
    }
    return logger;
}

}
