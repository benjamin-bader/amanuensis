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

#include <cerrno>
#include <memory>
#include <string>
#include <unordered_map>

#include <QApplication>
#include <QtTest>

#include "HttpMessageParserTests.h"

int main(int argc, char **argv)
{
    QApplication application(argc, argv);
    QStringList arguments = QCoreApplication::arguments();

    std::unordered_map<std::string, std::unique_ptr<QObject>> tests;
    tests.emplace("RequestParser", std::make_unique<HttpMessageParserTests>());

    int status = 0;
    for (auto& kvp : tests)
    {
        // QApplication's ctor sometimes leaves errno set to EINVAL.
        // Some tests may also alter errno.  In any case, let's start
        // our tests with a blank slate.
        errno = 0;

        auto& test = kvp.second;
        status |= QTest::qExec(test.get(), arguments);
    }
    return status;
}
