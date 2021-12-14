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

#include "RequestTest.h"

#include <QtTest>

#include "core/Request.h"

using namespace ama;

void RequestTest::format_simple_get()
{
    Request request;
    request.set_major_version(1);
    request.set_minor_version(1);
    request.set_method("GET");
    request.set_uri("http://www.google.com/?q=foo");
    request.headers().insert("Host", "www.google.com");
    request.headers().insert("User-Agent", "Qt Unit Test");

    QByteArray expected = "GET http://www.google.com/?q=foo HTTP/1.1\r\n"
            "Host: www.google.com\r\n"
            "User-Agent: Qt Unit Test\r\n"
            "\r\n";

    QByteArray actual = request.format();

    QCOMPARE(actual, expected);
}

void RequestTest::format_simple_post()
{
    Request request;
    request.set_major_version(1);
    request.set_minor_version(1);
    request.set_method("POST");
    request.set_uri("http://www.google.com/upload_to_me_plz");
    request.headers().insert("Host", "www.google.com");
    request.headers().insert("User-Agent", "Qt Unit Test");
    request.headers().insert("Content-Type", "application/json");
    request.headers().insert("Content-Length", "14");
    request.set_body(QByteArray{R"({"foo": "bar"})"});

    QByteArray expected = "POST http://www.google.com/upload_to_me_plz HTTP/1.1\r\n"
            "Host: www.google.com\r\n"
            "User-Agent: Qt Unit Test\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: 14\r\n"
            "\r\n"
            "{\"foo\": \"bar\"}";

    QByteArray actual = request.format();

    QCOMPARE(actual, expected);
}


QTEST_GUILESS_MAIN(RequestTest)
