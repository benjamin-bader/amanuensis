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

#include <cstdint>
#include <memory>
#include <string>
#include <system_error>

#ifdef _WIN32
#
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#
#  ifdef LOG_LIBRARY
#    define L_EXPORT __declspec(dllexport)
#  else
#    define L_EXPORT __declspec(dllimport)
#  endif // LOG_LIBRARY
#else
#  define L_EXPORT __attribute__((visibility("default")))
#endif // _WIN32


namespace ama::log {

enum class Severity : uint8_t
{
    Verbose = 0,
    Debug,
    Info,
    Warn,
    Error,
    Fatal = 0xFF
};

template <typename T>
class LogValue;

class L_EXPORT LogValueVisitor
{
public:
    virtual ~LogValueVisitor() = default;

    virtual void visit(const LogValue<bool>& value) noexcept = 0;
    virtual void visit(const LogValue<const char*>& value) noexcept = 0;
    virtual void visit(const LogValue<const wchar_t*>& value) noexcept = 0;
    virtual void visit(const LogValue<std::string>& value) noexcept = 0;
    virtual void visit(const LogValue<std::wstring>& value) noexcept = 0;
    virtual void visit(const LogValue<char>& value) noexcept = 0;
    virtual void visit(const LogValue<signed char>& value) noexcept = 0;
    virtual void visit(const LogValue<unsigned char>& value) noexcept = 0;
    virtual void visit(const LogValue<short>& value) noexcept = 0;
    virtual void visit(const LogValue<unsigned short>& value) noexcept = 0;
    virtual void visit(const LogValue<int>& value) noexcept = 0;
    virtual void visit(const LogValue<unsigned int>& value) noexcept = 0;
    virtual void visit(const LogValue<long>& value) noexcept = 0;
    virtual void visit(const LogValue<unsigned long>& value) noexcept = 0;
    virtual void visit(const LogValue<long long>& value) noexcept = 0;
    virtual void visit(const LogValue<unsigned long long>& value) noexcept = 0;
};

class L_EXPORT ILogValue // todo: suppress vtable?
{
public:
    virtual ~ILogValue() = default;
    virtual void accept(LogValueVisitor& visitor) const = 0;
    virtual const char* name() const { return nullptr; }
};

template <typename T>
class LogValue : public ILogValue
{
public:
    LogValue(const char* name, const T& value) noexcept
        : name_(name)
        , value_(value)
    {
    }

    LogValue(const char* name, T&& value) noexcept
        : name_(name)
        , value_(std::move(value))
    {
    }

    LogValue(const LogValue<T>&) = default;
    LogValue(LogValue<T>&&) = default;

    LogValue& operator=(const LogValue<T>&) = default;
    LogValue& operator=(LogValue<T>&&) = default;

    void accept(LogValueVisitor& visitor) const override
    {
        visitor.visit(*this);
    }

    const char* name() const override
    {
        return name_;
    }

    const T& value() const noexcept
    {
        return value_;
    }

private:
    const char* name_;
    const T value_;
};

using BoolValue = LogValue<bool>;
using ShortValue = LogValue<short>;
using IntValue = LogValue<int>;
using LongValue = LogValue<long>;
using UShortValue = LogValue<unsigned short>;
using UIntValue = LogValue<unsigned int>;
using ULongValue = LogValue<unsigned long>;
using I8Value = LogValue<int8_t>;
using I16Value = LogValue<int16_t>;
using I32Value = LogValue<int32_t>;
using I64Value = LogValue<int64_t>;
using U8Value = LogValue<uint8_t>;
using U16Value = LogValue<uint16_t>;
using U32Value = LogValue<uint32_t>;
using U64Value = LogValue<uint64_t>;
using SizeValue = LogValue<size_t>;
using CStrValue = LogValue<const char*>;
using StringValue = LogValue<std::string>;

#ifdef _WIN32

class L_EXPORT LastErrorValue : public ILogValue
{
public:
    LastErrorValue()
        : last_error_(GetLastError())
    {}

    const char* name() const override
    {
        return "LastError";
    }

    void accept(LogValueVisitor &visitor) const override
    {
        char buffer[256] = {};
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL,
                       last_error_,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       buffer,
                       255,
                       NULL);

        visitor.visit(CStrValue(name(), buffer));
    }

private:
    DWORD last_error_;
};

#endif

class L_EXPORT LogValueCollection : public ILogValue
{
public:
    LogValueCollection(const ILogValue** begin, const ILogValue**end)
        : begin_(begin)
        , end_(end)
    {}

    void accept(LogValueVisitor& visitor) const override
    {
        for (const ILogValue** logValue = begin_; logValue != end_; ++logValue)
        {
            (*logValue)->accept(visitor);
        }
    }

private:
    const ILogValue** begin_;
    const ILogValue** end_;
};

class L_EXPORT ILogWriter {
public:
    virtual ~ILogWriter() noexcept {}
    virtual void write(Severity severity, const char *message, const ILogValue& value) = 0;
};

L_EXPORT void register_log_writer(std::shared_ptr<ILogWriter>&& writer);

/**
 * @brief is_trace_enabled
 * @param severity
 * @return
 */
L_EXPORT bool is_enabled_for_severity(Severity severity);

/**
 * Logs a structured event with the given ILogValue.
 *
 * @param severity
 * @param message
 * @param structuredData
 */
L_EXPORT void do_log_event(Severity severity, const char *message, const ILogValue& structuredData);

/**
 * Logs a structured event with zero or more values.
 *
 * @param severity
 * @param message
 * @param values
 */
template <typename ...LogValues>
void log_event(Severity severity, const char* message, LogValues&&... values)
{
    if (! is_enabled_for_severity(severity))
    {
        return;
    }

    const ILogValue* valuesArray[sizeof...(LogValues)] = { std::addressof(values)... };
    const ILogValue** begin = &valuesArray[0];
    const ILogValue** end = begin + sizeof...(LogValues);

    LogValueCollection collection(begin, end);
    do_log_event(severity, message, collection);
}

template <typename ...LogValues>
void verbose(const char* message, LogValues&&... values)
{
    log_event(Severity::Verbose, message, std::forward<LogValues>(values)...);
}

template <typename ...LogValues>
void debug(const char* message, LogValues&&... values)
{
    log_event(Severity::Debug, message, std::forward<LogValues>(values)...);
}

template <typename ...LogValues>
void info(const char* message, LogValues&&... values)
{
    log_event(Severity::Info, message, std::forward<LogValues>(values)...);
}

template <typename ...LogValues>
void warn(const char* message, LogValues&&... values)
{
    log_event(Severity::Warn, message, std::forward<LogValues>(values)...);
}

template <typename ...LogValues>
void error(const char* message, LogValues&&... values)
{
    log_event(Severity::Error, message, std::forward<LogValues>(values)...);
}

} // ama::log
