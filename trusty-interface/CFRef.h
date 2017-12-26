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

#ifndef CFREF_H
#define CFREF_H

#pragma once

#include <type_traits>

#include <CoreFoundation/CoreFoundation.h>

namespace ama { namespace trusty {

template <typename T>
class is_cfref : public std::false_type {};

template <> class is_cfref<CFTypeRef>              : public std::true_type {};
template <> class is_cfref<CFDictionaryRef>        : public std::true_type {};
template <> class is_cfref<CFMutableDictionaryRef> : public std::true_type {};
template <> class is_cfref<CFArrayRef>             : public std::true_type {};
template <> class is_cfref<CFStringRef>            : public std::true_type {};
template <> class is_cfref<CFMutableStringRef>     : public std::true_type {};
template <> class is_cfref<CFDataRef>              : public std::true_type {};
template <> class is_cfref<CFBooleanRef>           : public std::true_type {};
template <> class is_cfref<CFDateRef>              : public std::true_type {};
template <> class is_cfref<CFNumberRef>            : public std::true_type {};

/*! A simple RAII smart-pointer for Core Foundation references that
 *  are freed with CFRelease.
 */
template <typename T>
class CFRef
{
    static_assert(is_cfref<T>::value, "Type T must be a CFTypeRef type");

public:
    CFRef() : CFRef(nullptr)
    {
    }

    CFRef(T ref) : m_ref(ref)
    {
    }

    CFRef(CFRef<T>&& other)
    {
        m_ref = other.m_ref;
        other.m_ref = nullptr;
    }

    ~CFRef()
    {
        reset();
    }

    CFRef<T>& operator=(T other)
    {
        reset();
        m_ref = other;
        return *this;
    }

    T get() const noexcept
    {
        return m_ref;
    }

    CFTypeID get_type_id() const noexcept
    {
        return CFGetTypeID(m_ref);
    }

    operator T() noexcept
    {
        return m_ref;
    }

    operator bool() noexcept
    {
        return m_ref != nullptr;
    }

    virtual void reset()
    {
        if (m_ref != nullptr)
        {
            CFRelease(m_ref);
            m_ref = nullptr;
        }
    }

private:
    T m_ref;
};

}} // ama::trusty

#endif // CFREF_H
