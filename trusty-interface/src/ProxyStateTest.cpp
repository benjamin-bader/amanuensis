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

#include "ProxyStateTest.h"

#include <string>
#include <sstream>

#include <QString>
#include <QtTest>

#include "trusty/ProxyState.h"

using namespace ama::trusty;

ProxyStateTest::ProxyStateTest()
{
}

void ProxyStateTest::serialize()
{
    ProxyState state(true, "google.com", 0x0000FFFF);
    std::vector<uint8_t> expected{1, 0x00, 0x00, 0xFF, 0xFF, 'g', 'o', 'o', 'g', 'l', 'e', '.', 'c', 'o', 'm' };
    std::vector<uint8_t> actual = state.serialize();
    QCOMPARE(expected, actual);
}

void ProxyStateTest::deserialize_empty_host()
{
    std::vector<uint8_t> serialized{0, 0, 0, 0, 0}; // disabled, port=0, host=""
    ProxyState state{serialized};
    QCOMPARE(std::string{""}, state.get_host());
    QCOMPARE(false, state.is_enabled());
    QCOMPARE(0, state.get_port());
}

QTEST_GUILESS_MAIN(ProxyStateTest)
