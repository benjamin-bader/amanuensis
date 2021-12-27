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
