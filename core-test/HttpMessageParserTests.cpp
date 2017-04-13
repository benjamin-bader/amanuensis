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

#include "HttpMessageParserTests.h"

#include <string>
#include <sstream>

#include <QString>
#include <QtTest>

#include "Headers.h"
#include "HttpMessage.h"
#include "HttpMessageParser.h"

HttpMessageParserTests::HttpMessageParserTests()
{
}

void HttpMessageParserTests::simpleGet()
{
    std::stringstream requestText;
    requestText << "GET /foo/bar HTTP/1.1\r\n";
    requestText << "Accept: application/html\r\n";
    requestText << "\r\n";

    HttpMessage request;
    HttpMessageParser parser;
    parser.resetForRequest();

    auto content = requestText.str();
    auto begin = content.begin();
    auto end = content.end();

    auto state = parser.parse(request, begin, end);

    QCOMPARE(state, HttpMessageParser::State::Valid);

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
    QCOMPARE(header.first, {"Accept"});
    QCOMPARE(header.second, {"application/html"});
}

void HttpMessageParserTests::fixedLengthSimplePost()
{
    std::stringstream requestText;
    requestText << "POST /foo/bar HTTP/1.1\r\n";
    requestText << "Accept: application/html\r\n";
    requestText << "Content-Type: text/plain\r\n";
    requestText << "Content-Length: 12\r\n";
    requestText << "Transfer-Encoding: identity\r\n"; // tricky!
    requestText << "\r\n";
    requestText << "abcdefghijkl\r\n";

    HttpMessage request;
    HttpMessageParser parser;
    parser.resetForRequest();

    auto content = requestText.str();
    auto begin = content.begin();
    auto end = content.end();

    auto state = parser.parse(request, begin, end);

    QCOMPARE(state, HttpMessageParser::State::Valid);

    std::string expected("abcdefghijkl");
    std::string actual = request.body_as_string();
    QCOMPARE(actual, expected);
}

void HttpMessageParserTests::chunkedSimplePost()
{
    std::stringstream requestText;
    requestText << "POST /foo/bar HTTP/1.1\r\n";
    requestText << "Accept: application/html\r\n";
    requestText << "Content-Type: text/plain\r\n";
    requestText << "Transfer-Encoding: gzip, chunked\r\n";
    requestText << "\r\n";
    requestText << "5\r\n";
    requestText << "abcde\r\n";
    requestText << "9\r\n";
    requestText << "fghijklmn\r\n";
    requestText << "A\r\n";
    requestText << "opqrstuvwx\r\n";
    requestText << "c\r\n";
    requestText << "yz0123456789\r\n";
    requestText << "0\r\n";
    requestText << "\r\n";

    HttpMessage request;
    HttpMessageParser parser;
    parser.resetForRequest();

    auto content = requestText.str();
    auto begin = content.begin();
    auto end = content.end();

    auto state = parser.parse(request, begin, end);

    QCOMPARE(state, HttpMessageParser::State::Valid);

    std::string expected("abcdefghijklmnopqrstuvwxyz0123456789");
    std::string actual = request.body_as_string();
    QCOMPARE(actual, expected);

    QCOMPARE(request.method(), {"POST"});
    QCOMPARE(request.uri(), {"/foo/bar"});

    QCOMPARE(request.headers().find_by_name("Accept")->second, {"application/html"});
    QCOMPARE(request.headers().find_by_name("Content-Type")->second, {"text/plain"});
    QCOMPARE(request.headers().find_by_name("Transfer-Encoding")->second, {"gzip, chunked"});
}

void HttpMessageParserTests::simpleOkResponse()
{
    std::stringstream responseText;
    responseText << "HTTP/1.1 200 OK\r\n";
    responseText << "Content-Type: text/plain\r\n";
    responseText << "Content-Length: 5\r\n";
    responseText << "\r\n";
    responseText << "zzzzz\r\n";

    HttpMessage message;
    HttpMessageParser parser;
    parser.resetForResponse();

    auto content = responseText.str();
    auto begin = content.begin();
    auto end = content.end();

    auto state = parser.parse(message, begin, end);

    QCOMPARE(state, HttpMessageParser::State::Valid);

    QCOMPARE(message.status_code(), 200);
    QCOMPARE(message.status_message(), {"OK"});

    QCOMPARE(message.body_as_string(), {"zzzzz"});
}

