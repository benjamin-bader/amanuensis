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

#include "trusty/CFRef.h"
#include "trusty/ProxyState.h"

using namespace ama::trusty;

ProxyStateTest::ProxyStateTest()
{
}

void ProxyStateTest::to_xpc()
{
    ProxyState state(true, "google.com", 0x0000FFFF);
    XRef<xpc_object_t> actual = state.to_xpc();

    QCOMPARE(true, xpc_dictionary_get_bool(actual, "enabled"));
    QCOMPARE("google.com", xpc_dictionary_get_string(actual, "host"));
    QCOMPARE(0x0000FFFF, xpc_dictionary_get_int64(actual, "port"));
}

void ProxyStateTest::from_xpc()
{
    XRef<xpc_object_t> serialized = xpc_dictionary_create_empty();
    xpc_dictionary_set_bool(serialized, "enabled", false);
    xpc_dictionary_set_int64(serialized, "port", 0);
    xpc_dictionary_set_string(serialized, "host", "");

    ProxyState state{serialized};
    QCOMPARE(std::string{""}, state.get_host());
    QCOMPARE(false, state.is_enabled());
    QCOMPARE(0, state.get_port());
}

QTEST_GUILESS_MAIN(ProxyStateTest)
