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

#include "Headers.h"

#include <algorithm>
#include <cctype>

Headers::Headers() : std::vector<Header>()
{
}

Headers::Headers(const Headers &headers) : std::vector<Header>(static_cast<std::vector<Header>>(headers))
{
}

Headers::Headers(const std::vector<Header> &headers) : std::vector<Header>(headers)
{
}

Headers::const_iterator Headers::find_by_name(const std::string &name) const
{
    std::string needle;
    needle.reserve(name.size());

    std::transform(name.begin(), name.end(), needle.begin(), std::tolower);

    return std::find_if(begin(), end(), [&needle](const Header &header) {
        auto needleSize = needle.size();
        auto headerName = header.name();
        auto nameSize = headerName.size();

        if (needleSize != nameSize)
        {
            return false;
        }

        for (auto i = 0; i < needleSize; ++i)
        {
            if (std::tolower(headerName[i]) != needle[i])
            {
                return false;
            }
        }

        return true;
    });
}
