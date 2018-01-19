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

#ifndef STRINGSTREAMLOGVALUEVISITOR_H
#define STRINGSTREAMLOGVALUEVISITOR_H

#pragma once

#include <codecvt>
#include <locale>
#include <sstream>

#include "Log.h"

namespace ama { namespace log {

class L_EXPORT StringStreamLogValueVisitor : public LogValueVisitor
{
public:
    StringStreamLogValueVisitor()
        : ss_()
        , finished_(false)
    {
        ss_ << "[";
    }

    void visit(const LogValue<bool>& value) noexcept override
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<const char*>& value) noexcept override
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<const wchar_t*>& value) noexcept override
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<std::string>& value) noexcept override
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<std::wstring>& value) noexcept override
    {
        using cvt = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<cvt> converter;
        ss_ << " " << value.name() << "=" << converter.to_bytes(value.value());
    }

    void visit(const LogValue<int8_t>& value) noexcept override
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<int16_t>& value) noexcept override
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<int32_t>& value) noexcept override
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<int64_t>& value) noexcept override
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<uint8_t>& value) noexcept override
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<uint16_t>& value) noexcept override
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<uint32_t>& value) noexcept override
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    void visit(const LogValue<uint64_t>& value) noexcept override
    {
        ss_ << " " << value.name() << "=" << value.value();
    }

    std::string str()
    {
        if (!finished_)
        {
            ss_ << " ]";
            finished_ = true;
        }
        return ss_.str();
    }

private:
    std::stringstream ss_;
    bool finished_;
};

}}

#endif // STRINGSTREAMLOGVALUEVISITOR_H
