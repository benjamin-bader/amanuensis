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

#ifndef REQUESTTEST_H
#define REQUESTTEST_H

#include <QObject>

class RequestTest : public QObject
{
    Q_OBJECT

public:
    RequestTest() = default;

private Q_SLOTS:
    void format_simple_get();
    void format_simple_post();
};

#endif // REQUESTTEST_H
