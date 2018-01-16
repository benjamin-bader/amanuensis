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

INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../log \

LIBS += \
    -L$${OUT_PWD}/../log/ -llog \

DEFINES += \
    ONLY_C_LOCALE

DEFINES += \
    ASIO_STANDALONE \
    ASIO_HAS_STD_CHRONO \
    ASIO_HAS_MOVE \
    ASIO_HAS_STD_SYSTEM_ERROR

SOURCES += \
    Proxy.cpp \
    Server.cpp \
    Headers.cpp \
    ProxyFactory.cpp \
    HttpMessage.cpp \
    HttpMessageParser.cpp \
    ProxyTransaction.cpp \
    ConnectionPool.cpp \
    Request.cpp \
    Response.cpp \
    Errors.cpp \
    Logging.cpp \
    Dates.cpp

HEADERS += \
    Proxy.h \
    global.h \
    Server.h \
    Headers.h \
    ProxyFactory.h \
    ObjectPool.h \
    HttpMessage.h \
    HttpMessageParser.h \
    Listenable.h \
    asiofwd.h \
    Transaction.h \
    ProxyTransaction.h \
    ConnectionPool.h \
    common.h \
    Request.h \
    Response.h \
    Errors.h \
    Logging.h \
    Dates.h

windows {
    SOURCES += \
        win/RegistryKey.cpp \
        win/WindowsProxy.cpp \
        win/OutputDebugStringSink.cpp \

    HEADERS += \
        win/RegistryKey.h \
        win/WindowsProxy.h \
        win/OutputDebugStringSink.h \

    LIBS += -lwininet
}

macx {
    # Silence Clang warnings about ASIO by marking it as a "system" include.
    QMAKE_CXXFLAGS += \
        -isystem $$PWD/../include

    QMAKE_CXXFLAGS += -Wno-unused-local-typedef

    SOURCES += \
        mac/OsLogSink.cpp \

    HEADERS += \
        mac/OsLogSink.h \
}

unix {
    target.path = /usr/lib
    INSTALLS += target
}
