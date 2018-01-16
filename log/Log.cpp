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
#include <string>
#include <sstream>
#include <thread>

namespace ama { namespace log {

namespace {

volatile Severity g_min_severity = Severity::Debug;

std::ostream& operator<<(std::ostream& os, Severity severity)
{
    switch (severity)
    {
    case Severity::Verbose: return os << "V";
    case Severity::Debug: return os << "D";
    case Severity::Info: return os << "I";
    case Severity::Warn: return os << "W";
    case Severity::Error: return os << "E";
    case Severity::Fatal: return os << "F";
    default:
        return os << "Unknown severity (" << static_cast<uint8_t>(severity) << ")";
    }
}

class SimpleVisitor : public LogValueVisitor
{
public:
    SimpleVisitor(Severity severity, const char* msg)
    {
        ss_ << "[ tid=" << std::this_thread::get_id() << " sev=" << severity << " msg=\"" << msg << "\"";
    }

    void visit(const LogValue<bool>& value) noexcept
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<const char*>& value) noexcept
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<const wchar_t*>& value) noexcept
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<std::string>& value) noexcept
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<std::wstring>& value) noexcept
    {
        //ss_ << value.name() << ": " << value.value() << std::endl;
    }

    void visit(const LogValue<int8_t>& value) noexcept
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<int16_t>& value) noexcept
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<int32_t>& value) noexcept
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<int64_t>& value) noexcept
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<uint8_t>& value) noexcept
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<uint16_t>& value) noexcept
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<uint32_t>& value) noexcept
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<uint64_t>& value) noexcept
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    std::string str()
    {
        if (!finished_)
        {
            ss_ << " ]" << std::endl;
            finished_ = true;
        }
        return ss_.str();
    }

private:
    std::stringstream ss_;
    bool finished_ = false;

};

}

bool is_enabled_for_severity(Severity severity)
{
    return g_min_severity <= severity;
}

void do_log_event(Severity severity, const char *message, const ILogValue &structuredData)
{

    SimpleVisitor visitor(severity, message);
    structuredData.accept(visitor);
    std::cerr << visitor.str() << std::endl;
}

}}
