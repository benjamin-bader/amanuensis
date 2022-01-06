// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2022 Benjamin Bader
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

#include "HeadersTests.h"

#include "core/Headers.h"

#include <QList>
#include <QtTest>

#include <algorithm>
#include <functional>

using namespace ama;

void HeadersTests::namesAreCanonicalized()
{
    Headers headers;

    headers.insert("foo", "bar");
    headers.insert("baz-qUX", "quuz");

    QList<QString> expected;
    expected << "Foo" << "Baz-Qux";

    QCOMPARE(expected, headers.names());
}

void HeadersTests::multipleInsertionsOfOneName()
{
    using namespace std::placeholders;

    Headers headers;
    headers.insert("foo", "bar");
    headers.insert("Foo", "baz");
    headers.insert("FOO", "quux");

    QList<QString> expectedNames;
    QList<QString> expectedValues;

    expectedNames << "Foo";
    expectedValues << "bar" << "baz" << "quux";

    auto actualValues = headers.find_by_name("foo");
    auto containsFn = [&](const QString& value) { return actualValues.contains(value); };

    QCOMPARE(expectedNames, headers.names());
    QVERIFY2(std::all_of(expectedValues.begin(), expectedValues.end(), containsFn), "headers returns all expected values");
}

QTEST_GUILESS_MAIN(HeadersTests)
