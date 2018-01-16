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

#ifndef LOG_H
#define LOG_H

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <system_error>

namespace ama { namespace log {

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

class LogValueVisitor
{
public:
    virtual ~LogValueVisitor() = default;

    virtual void visit(const LogValue<bool>&) noexcept = 0;
    virtual void visit(const LogValue<const char*>&) noexcept = 0;
    virtual void visit(const LogValue<const wchar_t*>&) noexcept = 0;
    virtual void visit(const LogValue<std::string>&) noexcept = 0;
    virtual void visit(const LogValue<std::wstring>&) noexcept = 0;
    virtual void visit(const LogValue<int8_t>&) noexcept = 0;
    virtual void visit(const LogValue<int16_t>&) noexcept = 0;
    virtual void visit(const LogValue<int32_t>&) noexcept = 0;
    virtual void visit(const LogValue<int64_t>&) noexcept = 0;
    virtual void visit(const LogValue<uint8_t>&) noexcept = 0;
    virtual void visit(const LogValue<uint16_t>&) noexcept = 0;
    virtual void visit(const LogValue<uint32_t>&) noexcept = 0;
    virtual void visit(const LogValue<uint64_t>&) noexcept = 0;
};

class ILogValue // todo: suppress vtable?
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

    LogValue(const LogValue<T>& that) noexcept
    {
        this->name_ = that.name_;
        this->value_ = that.value_;
    }

    LogValue(LogValue<T>&& that) noexcept
    {
        this->name_ = that.name_;
        this->value_ = std::move(that.value_);
        that.name_ = nullptr;
    }

    LogValue& operator=(const LogValue<T>& that) noexcept
    {
        this->name_ = that.name_;
        this->value_ = that.value_;
        return *this;
    }

    LogValue& operator=(LogValue<T>&& that) noexcept
    {
        this->name_ = that.name_;
        this->value_ = std::move(that.value_);
        that.name_ = nullptr;
        return *this;
    }

    void accept(LogValueVisitor& visitor) const override
    {
        visitor.visit(*this);
    }

    const char* name() const override
    {
        return name_;
    }

    const T& value() const
    {
        return value_;
    }

private:
    const char* name_;
    const T value_;
};

using TracedBool = LogValue<bool>;
using TracedShort = LogValue<short>;
using TracedInt = LogValue<int>;
using TracedLong = LogValue<long>;
using TracedUShort = LogValue<unsigned short>;
using TracedUInt = LogValue<unsigned int>;
using TracedULong = LogValue<unsigned long>;
using TracedI8 = LogValue<int8_t>;
using TracedI16 = LogValue<int16_t>;
using TracedI32 = LogValue<int32_t>;
using TracedI64 = LogValue<int64_t>;
using TracedU8 = LogValue<uint8_t>;
using TracedU16 = LogValue<uint16_t>;
using TracedU32 = LogValue<uint32_t>;
using TracedU64 = LogValue<uint64_t>;
using TracedCStr = LogValue<const char*>;
using TracedString = LogValue<std::string>;

class LogValueCollection : public ILogValue
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

/**
 * @brief is_trace_enabled
 * @param severity
 * @return
 */
bool is_enabled_for_severity(Severity severity);

/**
 * Logs a structured event with the given ILogValue.
 *
 * @param severity
 * @param message
 * @param structuredData
 */
void do_log_event(Severity severity, const char *message, const ILogValue& structuredData);

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

    std::array<const ILogValue*, sizeof...(LogValues)> valuesArray = { { std::addressof(values)... } };
    LogValueCollection collection(valuesArray.data(), valuesArray.data() + sizeof...(LogValues));
    do_log_event(severity, message, collection);
}

}} // ama::log

#endif // LOG_H
