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

#ifndef ERRORS_H
#define ERRORS_H

#pragma once

#include <string>
#include <system_error>

#include "global.h"

namespace ama
{

enum class ProxyError
{
    NetworkError = 1,
    RemoteDnsLookupError,
    ClientDisconnected,
    RemoteDisconnected,
    MalformedRequest,
    MalformedResponse,
};

enum class ProxyErrorType
{
    SessionFatal = 0,
    GlobalFatal = 0,
};

} // namespace ama

namespace std
{

template <>
struct is_error_code_enum<ama::ProxyError> : true_type {};

template <>
struct is_error_condition_enum<ama::ProxyErrorType> : true_type {};

std::error_code make_error_code(ama::ProxyError);

} // namespace std

#endif // ERRORS_H
