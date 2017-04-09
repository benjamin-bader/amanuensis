# Amanuensis - Web Traffic Inspector
#
# Copyright (C) 2017 Benjamin Bader
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

QT       += network
QT       -= gui

TARGET = core
TEMPLATE = lib

CONFIG += c++14

DEFINES += CORE_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

#DEFINES += DEBUG_PARSER_TRANSITIONS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += $$PWD/../include $$PWD/include

DEFINES += ASIO_STANDALONE ASIO_HAS_STD_CHRONO ASIO_HAS_MOVE

SOURCES += Amanuensis.cpp \
    Proxy.cpp \
    Server.cpp \
    Connection.cpp \
    Request.cpp \
    Headers.cpp \
    Response.cpp \
    ConnectionManager.cpp \
    ProxyFactory.cpp \
    RequestParser.cpp

HEADERS += Amanuensis.h \
    Proxy.h \
    global.h \
    Server.h \
    Connection.h \
    Request.h \
    Headers.h \
    Response.h \
    ConnectionManager.h \
    ProxyFactory.h \
    RequestParser.h

windows {
    SOURCES += \
        win/RegistryKey.cpp \
        win/WindowsProxy.cpp

    HEADERS += \
        win/RegistryKey.h \
        win/WindowsProxy.h

    LIBS += -lwininet
}

macx {
    message(hi)
}

unix {
    target.path = /usr/lib
    INSTALLS += target
}
