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

#include "BytesTest.h"

#include <cstdint>
#include <vector>

#include <QtTest>

#include "Bytes.h"

using namespace ama::trusty;

BytesTest::BytesTest()
{
}

void BytesTest::to_network_order()
{
    std::vector<uint8_t> v(4);

    Bytes::to_network_order<int32_t>(257, v.data());
    QCOMPARE((std::vector<uint8_t>{0, 0, 1, 1}), v);

    Bytes::to_network_order<uint32_t>(0xAABBCCDD, v.data());
    QCOMPARE((std::vector<uint8_t>{ 0xAA, 0xBB, 0xCC, 0xDD }), v);
}

void BytesTest::from_network_order()
{
    std::vector<uint8_t> bytes;

    bytes = { 0, 0, 1, 1 };
    QCOMPARE(257, Bytes::from_network_order<int32_t>(bytes.data()));

    bytes = { 0xAA, 0xBB, 0xCC, 0xDD };
    QCOMPARE(0xAABBCCDD, Bytes::from_network_order<uint32_t>(bytes.data()));

    bytes = { 0, 0, 0, 0, 0xFF };
    QCOMPARE(0, Bytes::from_network_order<int32_t>(bytes.data()));
}

QTEST_GUILESS_MAIN(BytesTest)
