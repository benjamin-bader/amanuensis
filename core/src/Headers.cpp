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

#include "core/Headers.h"

using namespace ama;

Headers::Headers()
    : values_()
    , insertion_order_()
{
}

bool Headers::empty() const
{
    return values_.empty();
}

size_t Headers::size() const
{
    return static_cast<size_t>(values_.size());
}

void Headers::insert(const QString& name, const QString& value)
{
    auto lowerName = name.toLower();
    if (!values_.contains(lowerName))
    {
        insertion_order_.append(lowerName);
    }
    values_.insert(lowerName, value);
}

QList<QString> Headers::find_by_name(const QString& name) const
{
    return values_.values(name.toLower());
}

QList<QString> Headers::names() const
{
    QList<QString> result;
    result.reserve(insertion_order_.size());

    for (auto it = insertion_order_.begin(); it != insertion_order_.end(); ++it)
    {
        QString name(*it);

        // normalize it
        bool first = true;
        for (qsizetype i = 0; i < name.size(); ++i)
        {
            QChar c = name.at(i);
            if (c.isLetter())
            {
                if (first)
                {
                    name[i] = c.toUpper();
                    first = false;
                }
                else
                {
                    name[i] = c.toLower();
                }
            }
            else if (c == '-')
            {
                first = true;
            }
        }

        result.append(name);
    }

    return result;
}
