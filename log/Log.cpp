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

#include "Log.h"

#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "StringStreamLogValueVisitor.h"

namespace ama { namespace log {

namespace {

std::string severity_name(Severity severity)
{
    switch (severity)
    {
    case Severity::Verbose: return "V";
    case Severity::Debug: return "D";
    case Severity::Info: return "I";
    case Severity::Warn: return "W";
    case Severity::Error: return "E";
    case Severity::Fatal: return "F";
    default:
        {
            std::stringstream ss;
            ss << "Unknown severity (" << static_cast<uint8_t>(severity) << ")";
            return ss.str();
        }
    }
}

class SimpleLogWriter : public ILogWriter
{
public:
    void write(Severity severity, const char *message, const ILogValue &value) override
    {
        StringStreamLogValueVisitor visitor;
        visitor.visit(StringValue("sev", severity_name(severity)));
        visitor.visit(CStrValue("msg", message));
        value.accept(visitor);
        std::cerr << visitor.str() << std::endl;
    }
};

volatile Severity g_min_severity = Severity::Debug;

std::mutex g_writer_lock;
std::shared_ptr<ILogWriter> g_writer = std::make_shared<SimpleLogWriter>();

} // namespace

void register_log_writer(std::shared_ptr<ILogWriter>&& writer)
{
    std::lock_guard<std::mutex> lock(g_writer_lock);
    g_writer = std::move(writer);
}

bool is_enabled_for_severity(Severity severity)
{
    return g_min_severity <= severity;
}

void do_log_event(Severity severity, const char *message, const ILogValue &structuredData)
{
    std::lock_guard<std::mutex> lock(g_writer_lock);
    g_writer->write(severity, message, structuredData);
}

}} // ama::log
