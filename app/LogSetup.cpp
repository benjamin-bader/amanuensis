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

#include "LogSetup.h"

#include <QtGlobal>

#include "QtLogWriter.h"

#include "mac/MacLogSetup.h"
#include "win/WindowsLogSetup.h"

#include "log/Log.h"

namespace ama {

class GenericLogSetup : public LogSetup
{
public:
    void configure_logging() override
    {
        log::register_log_writer(std::make_shared<QtLogWriter>());
    }
};

std::unique_ptr<LogSetup> make_log_configurer()
{
#if defined(Q_OS_WIN)
    return std::make_unique<WindowsLogSetup>();
#elif defined(Q_OS_DARWIN)
    return std::make_unique<MacLogSetup>();
#else
    return std::make_unique<GenericLogSetup>();
#endif
}

}
