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

#include "RequestParserTests.h"

#include <string>
#include <sstream>

#include <QString>
#include <QtTest>

#include "Headers.h"
#include "Request.h"
#include "RequestParser.h"

RequestParserTests::RequestParserTests()
{
}

void RequestParserTests::simpleGet()
{
    std::stringstream requestText;
    requestText << "GET /foo/bar HTTP/1.1\r\n";
    requestText << "Accept: application/html\r\n";
    requestText << "\r\n";

    Request request;
    RequestParser parser;

    auto content = requestText.str();
    auto begin = content.begin();
    auto end = content.end();

    auto state = parser.parse(request, begin, end);

    QCOMPARE(state, RequestParser::State::Valid);

    QCOMPARE(request.method(), std::string("GET"));
    QCOMPARE(request.uri(), std::string("/foo/bar"));
    QCOMPARE(request.major_version(), 1);
    QCOMPARE(request.minor_version(), 1);

    auto headers = request.headers();
    auto headerFindResult = headers.find_by_name("accept"); // case-insensitive find
    if (headerFindResult == headers.end())
    {
        QFAIL("Expected an Accept header, but none was found");
    }

    auto header = *headerFindResult;
    QCOMPARE(header.name(), std::string("Accept"));
    QCOMPARE(header.value(), std::string("application/html"));
}

