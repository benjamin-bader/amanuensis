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

#ifndef TRUSTYCOMMON_H
#define TRUSTYCOMMON_H

#pragma once

#include <cstdint>
#include <exception>
#include <string>

namespace ama
{
    extern const uint32_t kToolVersion;

    extern const std::string kHelperLabel;
    extern const std::string kHostAppRightName;
    extern const std::string kHelperSocketPath;

    extern const char * kGetProxyStateRightName;
    extern const char * kSetProxyStateRightName;
    extern const char * kSetHostRightName;
    extern const char * kSetPortRightName;
    extern const char * kGetHostRightName;
    extern const char * kGetPortRightName;
    extern const char * kClearSettingsRightName;
    extern const char * kGetToolVersionRightName;

    extern const char * kPlistLaunchdSocketName;

    class timeout_exception : public std::runtime_error
    {
    public:
        timeout_exception()
            : std::runtime_error("An operation timed out")
        {}

        timeout_exception(const timeout_exception &other)
            : std::runtime_error(other.what())
        {}
    };
}

#endif
