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

#pragma once

#include <string>
#include <vector>

#include "core/global.h"

namespace ama
{

class A_EXPORT Headers
{
private:
    std::vector<std::string> names_;
    std::vector<std::string> values_;

public:
    Headers();
    Headers(const Headers &headers);

    std::vector<std::string> find_by_name(const std::string &name) const;

    Headers normalize() const;

    bool empty() const;
    size_t size() const;

    const std::vector<std::string>& names() const { return names_; }
    const std::vector<std::string>& values() const { return values_; }

    void insert(const std::string &name, const std::string &value);
};

} // namespace ama
