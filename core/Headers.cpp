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
#include <climits>
#include <locale>
#include <map>
#include <mutex>
#include <sstream>

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

    struct eq_char
    {
        eq_char()
        {
            std::call_once(lowercase_init_flag, &init_lookup_table);
        }

        bool operator()(char x, char y) const
        {
            char lower_x = lookup_lower(x);
            char lower_y = lookup_lower(y);

            return lower_x == lower_y;
        }
    };

    const eq_char eq_char_instance = {};

    bool equals_case_insensitive(const std::string &lhs, const std::string &rhs)
    {
        return lhs.size() == rhs.size() &&
                std::equal(std::begin(lhs), std::end(lhs),
                           std::begin(rhs), std::end(rhs),
                           eq_char_instance);
    }

    struct ci_less
    {
        bool operator()(const std::string &lhs, const std::string &rhs) const
        {
            std::call_once(lowercase_init_flag, &init_lookup_table);

            return std::lexicographical_compare(lhs.begin(), lhs.end(),
                                                rhs.begin(), rhs.end(),
                                                lt_char());
        }
    };
}

Headers::Headers()
    : names_()
    , values_()
{
}

Headers::Headers(const Headers &headers)
    : names_(headers.names_)
    , values_(headers.values_)
{
}

size_t Headers::size() const
{
    return names_.size();
}

bool Headers::empty() const
{
    return names_.empty();
}

void Headers::insert(const std::string &name, const std::string &value)
{
    names_.push_back(name);
    values_.push_back(value);
}

std::vector<std::string> Headers::find_by_name(const std::string &name) const
{
    std::vector<std::string> result;

    for (size_t i = 0; i < names_.size(); ++i)
    {
        auto header_name  = names_[i];
        auto header_value = values_[i];

        if (equals_case_insensitive(name, header_name))
        {
            result.push_back(header_value);
        }
    }

    return result;
}

Headers Headers::normalize() const
{
    std::multimap<std::string, std::string, ci_less> map;

    for (size_t i = 0; i < size(); ++i)
    {
        map.insert(std::make_pair(names_[i], values_[i]));
    }

    Headers copy;

    std::vector<std::string> names;
    for (auto it = map.begin(); it != map.end(); it = map.upper_bound(it->first)) // iterate each key once;
    {
        names.push_back(it->first);
    }

    for (auto &name : names)
    {
        bool appended = false;
        std::stringstream ss;
        for (auto it = map.find(name); it != map.end(); ++it)
        {
            if (appended)
            {
                ss << ", ";
            }

            ss << it->second;
            appended = true;
        }

        copy.insert(name, ss.str());
    }

    return copy;
}
