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
    auto iter = begin();
    while (iter != end())
    {
        auto headerName = iter->name();
        if (name.size() == headerName.size())
        {
            bool areEqual = true;
            for (size_t i = 0; i < name.size(); ++i)
            {
                if (std::tolower(name[i]) != std::tolower(headerName[i]))
                {
                    areEqual = false;
                    break;
                }
            }

            if (areEqual)
            {
                return iter;
            }
        }
        ++iter;
    }
    return iter;
}
