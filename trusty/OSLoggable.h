#ifndef OSLOGGABLE_H
#define OSLOGGABLE_H

#pragma once

#include <memory>
#include <sstream>
#include <string>

class SystemLogger
{
public:
    virtual ~SystemLogger() {}

    virtual void debug(const std::string &message) = 0;
    virtual void info(const std::string &message)  = 0;
    virtual void error(const std::string &message) = 0;
};

class LogStatement : public std::stringstream
{
public:
    ~LogStatement();

    LogStatement(LogStatement &&rhs);

private:
    friend class OSLoggable;

    enum level_t {
        debug,
        info,
        error
    };

    LogStatement(level_t level, const std::shared_ptr<SystemLogger> logger);

    level_t level;
    const std::weak_ptr<SystemLogger> weak_logger;
};

class OSLoggable
{
public:
    OSLoggable(const std::string &label, const std::string &category);

protected:
    LogStatement log_debug();
    LogStatement log_info();
    LogStatement log_error();

    std::shared_ptr<SystemLogger> logger() const;

private:
    OSLoggable() = delete;

    std::shared_ptr<SystemLogger> logger_;
};

class Log : public OSLoggable
{
public:
    Log() : OSLoggable("com.bendb.amanuensis.Trusty", "log") {}

    LogStatement debug()
    {
        return log_debug();
    }

    LogStatement info()
    {
        return log_info();
    }

    LogStatement error()
    {
        return log_error();
    }
};

#endif // OSLOGGABLE_H
