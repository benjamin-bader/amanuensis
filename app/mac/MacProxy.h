// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2017 Benjamin Bader
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

#ifndef MACPROXY_H
#define MACPROXY_H

#pragma once

#include <system_error>

#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFError.h>
#include <CoreFoundation/CFString.h>

#include "Proxy.h"

#include "TrustyCommon.h"

class MacProxy : public ama::Proxy
{
public:
    MacProxy(int port);
    virtual ~MacProxy();

    bool is_enabled() const;

    // Attempts to apply system-wide proxy settings.
    void enable(std::error_code &ec);

    // Attempts to disable system-wide proxy settings.
    void disable(std::error_code &ec);

    void say_hi();

private:
    void bless_helper_program(std::error_code &ec) const;

    bool get_installed_helper_info(CFDictionaryRef *pRef) const;

private:
    bool enabled_;
};

#endif
