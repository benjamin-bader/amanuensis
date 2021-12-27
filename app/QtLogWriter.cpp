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

#include "QtLogWriter.h"

#include "log/StringStreamLogValueVisitor.h"

#include <qlogging.h>

#include <string>

using namespace ama::log;

QtLogWriter::QtLogWriter() {
}

void QtLogWriter::write(Severity severity, const char *message, const ILogValue &value)
{
    StringStreamLogValueVisitor visitor;
    value.accept(visitor);

    QMessageLogger logger;
    switch (severity)
    {
    case Severity::Verbose:
    case Severity::Debug:
        logger.debug("%s %s\n", message, visitor.str().c_str());
        break;

    case Severity::Info:
        logger.info("%s %s\n", message, visitor.str().c_str());
        break;

    case Severity::Warn:
        logger.warning("%s %s\n", message, visitor.str().c_str());
        break;

    case Severity::Error:
        logger.critical("%s %s\n", message, visitor.str().c_str());
        break;

    case Severity::Fatal:
        logger.fatal("%s %s\n", message, visitor.str().c_str());
        break;
    }

}
