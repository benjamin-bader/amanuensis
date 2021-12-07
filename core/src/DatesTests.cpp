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

#include "DatesTests.h"

#include <chrono>
#include <string>

#include <QtTest>

#include "core/Dates.h"

using namespace ama;

// TODO: Validate the return values of these parsed dates
void DatesTests::parse_modern_date()
{
    time_point tp;
    QVERIFY(Dates::parse_http_date("Sun, 06 Nov 1994 08:49:37 GMT", tp));
}

void DatesTests::parse_legacy_date()
{
    time_point tp;
    QVERIFY(Dates::parse_http_date("Sunday, 06-Nov-94 08:49:37 GMT", tp));
}

void DatesTests::parse_asctime_date()
{
    time_point tp;
    QVERIFY(Dates::parse_http_date("Sun Nov  6 08:49:37 1994", tp));
}

void DatesTests::parse_invalid_input_returns_false()
{
    time_point tp;
    QVERIFY(! Dates::parse_http_date("ceci n'est pas un date", tp));
}

QTEST_GUILESS_MAIN(DatesTests)
