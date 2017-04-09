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
#include <QThread>

#include <exception>

#include "Proxy.h"
#include "ProxyFactory.h"
#include "Server.h"

int main(int argc, char *argv[])
{
    try
    {
        Server server(9999);

        QApplication a(argc, argv);
        MainWindow w;
        w.show();

        return a.exec();
    }
    catch (std::domain_error &error)
    {
        qCritical() << error.what();
    }
}
