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

using namespace ama;

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

    if (headerFindResult.empty())
    {
        QFAIL("Expected an Accept header, but none was found");
    }

    auto value = headerFindResult[0];
    QCOMPARE(value, {"application/html"});
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

    QCOMPARE(request.headers().find_by_name("Accept")[0], {"application/html"});
    QCOMPARE(request.headers().find_by_name("Content-Type")[0], {"text/plain"});
    QCOMPARE(request.headers().find_by_name("Transfer-Encoding")[0], {"gzip, chunked"});
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

void HttpMessageParserTests::simpleForbiddenResponse()
{
    std::stringstream responseText;
    responseText << "HTTP/1.1 403 Forbidden\r\n";
    responseText << "Server: nginx\r\n";
    responseText << "Date: Fri, 14 Apr 2017 22:23:44 GMT\r\n";
    responseText << "Content-Type: text/html\r\n";
    responseText << "Transfer-Encoding: chunked\r\n";
    responseText << "Connection: keep-alive\r\n";
    responseText << "Vary: Accept-Encoding\r\n";
    responseText << "\r\n";
    responseText << "8\r\n";
    responseText << "<html>\r\n\r\n";
    responseText << "2B\r\n";
    responseText << "<head><title>403 Forbidden</title></head>\r\n\r\n";
    responseText << "18\r\n";
    responseText << "<body bgcolor=\"white\">\r\n\r\n";
    responseText << "29\r\n";
    responseText << "<center><h1>403 Forbidden</h1></center>\r\n\r\n";
    responseText << "1C\r\n";
    responseText << "<hr><center>nginx</center>\r\n\r\n";
    responseText << "9\r\n";
    responseText << "</body>\r\n\r\n";
    responseText << "9\r\n";
    responseText << "</html>\r\n\r\n";
    responseText << "0\r\n";
    responseText << "\r\n";

    HttpMessage message;
    HttpMessageParser parser;
    parser.resetForResponse();

    auto content = responseText.str();
    auto begin = content.begin();
    auto end = content.end();

    auto state = parser.parse(message, begin, end);

    QCOMPARE(state, HttpMessageParser::State::Valid);

    QCOMPARE(message.status_code(), 403);
    QCOMPARE(message.status_message(), {"Forbidden"});

    std::stringstream expected;
    expected << "<html>\r\n";
    expected << "<head><title>403 Forbidden</title></head>\r\n";
    expected << "<body bgcolor=\"white\">\r\n";
    expected << "<center><h1>403 Forbidden</h1></center>\r\n";
    expected << "<hr><center>nginx</center>\r\n";
    expected << "</body>\r\n";
    expected << "</html>\r\n";

    QCOMPARE(message.body_as_string(), expected.str());
}

void HttpMessageParserTests::connectFromEdge()
{
    std::string content =
            "CONNECT news.ycombinator.com:443 HTTP/1.0\r\n"
            "User-Agent: Mozilla/5.0 (Windows NT 10.0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36 Edge/12.10136\r\n"
            "Content-Length: 0\r\n"
            "Proxy-Connection: keep-alive\r\n"
            "Pragma: wtf\r\n"
            "\r\n";

    HttpMessage request;
    HttpMessageParser parser;
    parser.resetForRequest();

    auto begin = content.begin();
    auto end = content.end();

    auto state = parser.parse(request, begin, end);

    QCOMPARE(state, HttpMessageParser::State::Valid);
    QCOMPARE(request.method(), {"CONNECT"});
}

void HttpMessageParserTests::pauses_on_phase_transitions()
{
    std::string url = "https://www.smbc-comics.com/comic/the-talk-3";

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

    ParsePhase phase = ParsePhase::Start;
    auto state = parser.parse(request, begin, end, phase);

    QCOMPARE(HttpMessageParser::State::Incomplete, state);
    QCOMPARE(ParsePhase::ReceivedMessageLine, phase);
    QCOMPARE(std::string{"POST"}, request.method());
    QCOMPARE(std::string{"/foo/bar"}, request.uri());
    QCOMPARE(size_t(0), request.headers().size());
    QCOMPARE(std::string{""}, request.body_as_string());

    state = parser.parse(request, begin, end, phase);

    QCOMPARE(HttpMessageParser::State::Incomplete, state);
    QCOMPARE(ParsePhase::ReceivedHeaders, phase);
    QCOMPARE(size_t(3), request.headers().size());
    QCOMPARE(std::string{""}, request.body_as_string());

    state = parser.parse(request, begin, end, phase);

    QCOMPARE(HttpMessageParser::State::Valid, state);
    QCOMPARE(ParsePhase::ReceivedFullMessage, phase);
    QCOMPARE(std::string{"abcdefghijklmnopqrstuvwxyz0123456789"}, request.body_as_string());
}

