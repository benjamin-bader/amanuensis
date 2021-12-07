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

#pragma once

#include "log/Log.h"

#ifndef __APPLE__
#error "Can't use os_log on non-Apple platforms"
#endif

#include <os/log.h>

namespace ama::log {

class OsLogWriter : public ILogWriter
{
public:
    void write(Severity severity, const char* message, const ILogValue& value) override;

private:
    os_log_type_t log_type_for_severity(Severity severity);
};

} // ama::log