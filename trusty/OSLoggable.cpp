#include "OSLoggable.h"

#include <sstream>

// TODO(ben) Implement an ASL SystemLogger to target macOS < 10.12
#include <os/log.h>

class UnifiedSystemLogger : public SystemLogger
{
    os_log_t log;

public:
    UnifiedSystemLogger(const std::string &subsystem, const std::string &category)
    {
        log = os_log_create(subsystem.c_str(), category.c_str());
    }

    virtual ~UnifiedSystemLogger()
    {
        if (log != nullptr)
        {
            os_release(log);
        }
    }

    virtual void debug(const std::string &message) override
    {
        if (log != nullptr)
        {
            os_log_debug(log, message.c_str());
        }
    }

    virtual void info(const std::string &message) override
    {
        if (log != nullptr)
        {
            os_log_info(log, message.c_str());
        }
    }

    virtual void error(const std::string &message) override
    {
        if (log != nullptr)
        {
            os_log_error(log, message.c_str());
        }
    }
};

LogStatement::LogStatement(level_t level, const std::shared_ptr<SystemLogger> logger) :
    std::stringstream(),
    level(level),
    weak_logger(std::weak_ptr<SystemLogger>(logger))
{}

LogStatement::LogStatement(LogStatement &&rhs) :
    std::stringstream(std::move(static_cast<std::stringstream&>(rhs))),
    level(rhs.level),
    weak_logger(rhs.weak_logger)
{}


LogStatement::~LogStatement()
{
    if (auto logger = weak_logger.lock())
    {
        switch (level)
        {
        case LogStatement::level_t::debug: logger->debug(str()); break;
        case LogStatement::level_t::info:  logger->info(str());  break;
        case LogStatement::level_t::error: logger->error(str()); break;
        }
    }
}

OSLoggable::OSLoggable(const std::string &label, const std::string &category) :
    logger_(std::make_shared<UnifiedSystemLogger>(label, category))
{}

LogStatement OSLoggable::log_debug()
{
    return LogStatement(LogStatement::debug, logger_);
}

LogStatement OSLoggable::log_info()
{
    return LogStatement(LogStatement::info, logger_);
}

LogStatement OSLoggable::log_error()
{
    return LogStatement(LogStatement::error, logger_);
}

std::shared_ptr<SystemLogger> OSLoggable::logger() const
{
    return logger_;
}
