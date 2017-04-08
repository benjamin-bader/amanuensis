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

#include <string>
#include <vector>

#include "global.h"

class A_EXPORT Header
{
public:
    Header(const std::string &name, const std::string &value)
        : name_(name), value_(value)
    {
    }

    std::string name() const
    {
        return this->name_;
    }

    std::string value() const
    {
        return this->value_;
    }

private:
    std::string name_;
    std::string value_;
};

class A_EXPORT Headers
{
public:
    typedef std::vector<Header>::const_iterator const_iterator;

    explicit Headers(const std::vector<Header> &headers);

    int size() const;

    const_iterator begin() const;
    const_iterator end() const;

    const Header& at(int index) const;
    const Header& operator[](int index) const;

    const_iterator find_by_name(const std::string &name) const;

private:
    std::vector<Header> headers_;
};

#endif // HEADERS_H
