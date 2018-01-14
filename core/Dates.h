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

#ifndef DATES_H
#define DATES_H

#pragma once

#include <optional>
#include <string>

#include "common.h"
#include "global.h"

namespace ama { namespace Dates {

/**
 * Parses the given text into a time_point, according
 * to RFC 7231's Date/Time Formats spec in section 7.1.1.1.
 *
 * @param text the text to be parsed.
 * @param tp when this method returns successfully, will contain the parsed value.
 * @return true if @c text was successfully parsed, otherwise false.
 */
std::optional<time_point> A_EXPORT parse_http_date(const std::string& text) noexcept;

}} // ama::Dates

#endif // DATES_H
