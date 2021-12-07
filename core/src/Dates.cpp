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

#include "core/Dates.h"

#include "date/date.h"

namespace ama { namespace Dates {

namespace {

// per RFC 7231, we MUST accept dates in the following formats:
// 1. IMF-fixdate (e.g. Sun, 06 Nov 1994 08:49:37 GMT)
// 2. RFC 859     (e.g. Sunday, 06-Nov-94 08:49:37 GMT)
// 3. asctime()   (e.g. Sun Nov  6 08:49:37 1994)

// see std::get_time for format-string documentation
constexpr const char* IMF_FIXDATE = "%a, %d %b %Y %H:%M:%S GMT";
constexpr const char* RFC_850     = "%a, %d-%b-%y %H:%M:%S GMT";
constexpr const char* ASCTIME     = "%a %b %d %H:%M:%S %Y";

} // namespace

bool parse_http_date(const std::string &text, time_point& tp) noexcept
{
    for (const auto& format : { IMF_FIXDATE, RFC_850, ASCTIME })
    {
        tp = {};
        std::istringstream input(text);

        input >> date::parse(format, tp);

        if (input)
        {
            return true;
        }
    }

    return false;
}

}} // ama::Dates
