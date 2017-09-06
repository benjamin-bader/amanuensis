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
#include <locale>
#include <mutex>

using namespace ama;

namespace {
    char lowercase_lookup_table[256];
    std::once_flag lowercase_init_flag;

    inline int index_for_char(char c)
    {
        return static_cast<int>(c) - CHAR_MIN;
    }

    void init_lookup_table()
    {
        std::locale ascii = std::locale::classic();

        char c = CHAR_MIN;
        while (true)
        {
            lowercase_lookup_table[index_for_char(c)] = std::tolower(c, ascii);
            if (c == CHAR_MAX)
            {
                break;
            }
            ++c;
        }
    }

    inline char lookup_lower(char c)
    {
        return lowercase_lookup_table[index_for_char(c)];
    }

    struct lt_char
    {
        bool operator()(char x, char y) const
        {
            char lower_x = lookup_lower(x);
            char lower_y = lookup_lower(y);

            return lower_x < lower_y;
        }
    };
}

bool Headers::ci_less::operator ()(const std::string &lhs, const std::string &rhs) const
{
    std::call_once(lowercase_init_flag, &init_lookup_table);

    return std::lexicographical_compare(lhs.begin(), lhs.end(),
                                        rhs.begin(), rhs.end(),
                                        lt_char());
}

Headers::Headers() :
    map_()
{
}

Headers::Headers(const Headers &headers) :
    map_(headers.map_)
{
}

Headers::const_iterator  Headers::begin() const
{
    return map_.begin();
}

Headers::const_iterator Headers::end() const
{
    return map_.end();
}

size_t Headers::size() const
{
    return map_.size();
}

bool Headers::empty() const
{
    return map_.empty();
}

void Headers::insert(const std::string &name, const std::string &value)
{
    map_.insert(MapType::value_type {name, value});
}

Headers::iterator Headers::find_by_name(const std::string &name)
{
    return map_.find(name);
}

Headers::const_iterator Headers::find_by_name(const std::string &name) const
{
    return map_.find(name);
}
