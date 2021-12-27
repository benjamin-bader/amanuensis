#pragma once

#include "log/Log.h"

class QtLogWriter : public ama::log::ILogWriter
{
public:
    QtLogWriter();
    virtual ~QtLogWriter() noexcept = default;

    void write(ama::log::Severity severity, const char *message, const ama::log::ILogValue& value);
};

