// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2021 Benjamin Bader
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

#include "TrustyServiceTests.h"

#include "TrustyService.h"

#include <QtTest>

using namespace ama::trusty;

void TrustyServiceTests::validateEmptyHostname()
{
    QCOMPARE(true, is_valid_hostname_or_empty(""));
}

void TrustyServiceTests::validateLocalhost()
{
    QCOMPARE(true, is_valid_hostname_or_empty("localhost"));
}

void TrustyServiceTests::validateOneTwentySevenOne()
{
    QCOMPARE(true, is_valid_hostname_or_empty("127.0.0.1"));
}

void TrustyServiceTests::validateNumericHostname()
{
    QCOMPARE(true, is_valid_hostname_or_empty("12345"));
}

void TrustyServiceTests::validateNameTooLong()
{
    std::string hostname;
    for (int i = 0; i < 25; i++)
    {
        hostname += "1234567890";
    }
    hostname += "1234";

    QCOMPARE(254, hostname.size());

    QCOMPARE(false, is_valid_hostname_or_empty(hostname));

    hostname.resize(253);
    QCOMPARE(true, is_valid_hostname_or_empty(hostname));
}

void TrustyServiceTests::validateNameStartingWithHyphen()
{
    QCOMPARE(false, is_valid_hostname_or_empty("-foo.com"));
}

QTEST_GUILESS_MAIN(TrustyServiceTests)
