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

#include "trusty/CFLog.h"

#include <cstdlib>

namespace ama::log {

CFStringValue::CFStringValue(const char* name, CFStringRef value)
    : m_name(name)
    , m_value(value)
{}

const char* CFStringValue::name() const
{
    return m_name;
}

void CFStringValue::accept(LogValueVisitor& visitor) const
{
    const char* chars = CFStringGetCStringPtr(m_value, kCFStringEncodingUTF8);
    bool shouldFree = false;
    if (chars == nullptr)
    {
        shouldFree = true;

        CFRange range = CFRangeMake(0, CFStringGetLength(m_value));
        CFIndex outSize = 0;
        CFIndex converted = CFStringGetBytes(m_value, range, kCFStringEncodingUTF8, 0, false, NULL, 0, &outSize);

        chars = (char*) std::malloc(sizeof(char) * outSize + 1);
        if (chars == NULL)
        {
            // uh oh
            return;
        }

        converted = CFStringGetBytes(m_value, range, kCFStringEncodingUTF8, 0, false, (UInt8*) chars, outSize, NULL);
        ((char*) chars)[outSize] = '\0';
    }

    visitor.visit(CStrValue(m_name, chars));

    if (shouldFree)
    {
        std::free((void*) chars);
    }
}

}
