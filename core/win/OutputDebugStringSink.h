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

#ifndef OSLOGSINK_H
#define OSLOGSINK_H

#pragma once

#include <mutex>

#include <spdlog/sinks/base_sink.h>

#include "global.h"

namespace ama {

class A_EXPORT OutputDebugStringSink : public spdlog::sinks::base_sink<std::mutex>
{
protected:
    void _sink_it(const spdlog::details::log_msg& msg) override;
    void _flush() override;
};

} // namespace ama::trusty

#endif // OSLOGSINK_H
