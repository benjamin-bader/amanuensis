#-------------------------------------------------
#
# Project created by QtCreator 2017-04-08T14:14:36
#
#-------------------------------------------------

QT       += core testlib

TARGET = core-test
CONFIG   += console testcase
CONFIG   -= app_bundle

TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++14

DEFINES += ASIO_STANDALONE ASIO_HAS_STD_CHRONO ASIO_HAS_MOVE

HEADERS += \
    HttpMessageParserTests.h \
    RequestTest.h \
    DatesTests.h

SOURCES += \
    main.cpp \
    HttpMessageParserTests.cpp \
    RequestTest.cpp \
    DatesTests.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

include(../amanuensis-common.pri)

includeNeighborLib(core)
includeNeighborLib(log)

INCLUDEPATH += \
    $$PWD/../core \
    $$PWD/../log \
    $$PWD/../include

DEPENDPATH += \
    $$PWD/../core \
    $$PWD/../log \
