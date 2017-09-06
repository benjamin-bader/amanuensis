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

#ifndef HEADERS_H
#define HEADERS_H

#pragma once

#include <map>
#include <string>

#include "global.h"

namespace ama
{

class A_EXPORT Headers
{
private:
    struct ci_less
    {
        bool operator()(const std::string &lhs, const std::string &rhs) const;
    };

    std::multimap<std::string, std::string, ci_less> map_;

public:
    typedef std::multimap<std::string, std::string, ci_less> MapType;
    typedef MapType::iterator iterator;
    typedef MapType::const_iterator const_iterator;

    Headers();
    Headers(const Headers &headers);

    iterator find_by_name(const std::string &name);
    const_iterator find_by_name(const std::string &name) const;

    const_iterator begin() const;
    const_iterator end() const;

    bool empty() const;
    size_t size() const;

    void insert(const std::string &name, const std::string &value);
};

} // namespace ama

#endif // HEADERS_H
