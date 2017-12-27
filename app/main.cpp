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

#include "MainWindow.h"
#include <QApplication>
#include <QDebug>
#include <QSettings>
#include <QThread>

#include <cerrno>
#include <exception>
#include <memory>

#include "HttpMessage.h"
#include "Proxy.h"
#include "ProxyFactory.h"
#include "Server.h"

//Q_DECLARE_METATYPE(std::shared_ptr<ama::Connection>)
//Q_DECLARE_METATYPE(ama::HttpMessage)

Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr)

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Amanuensis");
    QCoreApplication::setOrganizationDomain("bendb.com");
    QCoreApplication::setApplicationName("amanuensis");
    QCoreApplication::setApplicationVersion("0.1.0");

    try
    {
        //qRegisterMetaType<std::shared_ptr<ama::Connection>>();
        //qRegisterMetaType<ama::HttpMessage>();

        QApplication a(argc, argv);

        // QApplication pollutes errno on Windows if no debugger
        // is attached.  Fix that here!
        errno = 0;

        MainWindow w;
        w.show();

        return a.exec();
    }
    catch (std::domain_error &error)
    {
        qCritical() << error.what();
    }
}
