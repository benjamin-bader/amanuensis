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

#ifndef OSLOGSINK_H
#define OSLOGSINK_H

#pragma once

#include <array>
#include <mutex>

#include <os/log.h>

#include <spdlog/sinks/base_sink.h>

#include "global.h"

namespace ama {

/**
 * Uses the new-ish os_log interface.
 *
 * This is a copy of the file in trusty_interface, in a different namespace.
 * I want os_log support in trusty, the app, and corelib, but haven't
 * figured out how to cleanly share code across all three.  This will do
 * for today.
 */
class A_EXPORT OsLogSink : public spdlog::sinks::base_sink<std::mutex>
{
public:
    OsLogSink();
    OsLogSink(os_log_t log);

protected:
    void _sink_it(const spdlog::details::log_msg& msg) override;
    void _flush() override;

private:
    inline os_log_type_t log_type_for_level(const spdlog::level::level_enum level) const;

private:
    os_log_t log_;
    std::array<os_log_type_t, 7> log_types_by_level_;
};

} // namespace ama

#endif // OSLOGSINK_H
