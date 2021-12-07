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

template <typename T>
class CFRefTraits
{
    static_assert(is_cfref<T>::value, "T must be a CoreFoundation type.  Did you forget to specialize is_cfref<T>?");

public:
    static void release(T ref) noexcept
    {
        if (ref != nullptr)
        {
            CFRelease(ref);
        }
    }

    static T empty() noexcept
    {
        return nullptr;
    }

    static bool is_empty(T ref) noexcept
    {
        return ref != empty();
    }
};

template <typename T, typename TTraits>
class RefHolder
{
    using ThisType = RefHolder<T, TTraits>;

public:
    RefHolder() : ref_(TTraits::empty())
    {
    }

    RefHolder(T ref) : ref_(ref)
    {
    }

    RefHolder(ThisType&& other) : ref_(other.detach())
    {
    }

    ~RefHolder()
    {
        do_release();
    }

    void reset()
    {
        do_release();
    }

    T detach()
    {
        T ref = ref_;
        ref_ = TTraits::empty();
        return ref;
    }

    void attach(T ref)
    {
        if (ref != ref_)
        {
            TTraits::release(ref_);
            ref_ = ref;
        }
    }

    operator T() const noexcept
    {
        return ref_;
    }

    operator bool() const noexcept
    {
        return ! TTraits::is_empty(ref_);
    }

    T get() const noexcept
    {
        return ref_;
    }

    const ThisType& operator=(T ref)
    {
        reset();
        ref_ = ref;
        return *this;
    }

    ThisType& operator=(ThisType&& holder)
    {
        attach(holder.detach());
        return *this;
    }

private:
    RefHolder(ThisType&) = delete;
    RefHolder(const ThisType&) = delete;

    void do_release()
    {
        T ref = ref_;
        ref_ = TTraits::empty();
        TTraits::release(ref);
    }

    T ref_;
};

/*! A simple RAII smart-pointer for Core Foundation references that
 *  are freed with CFRelease.
 */
template <typename T>
using CFRef = RefHolder<T, CFRefTraits<T>>;

}} // ama::trusty

#endif // CFREF_H
