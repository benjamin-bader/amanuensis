#-------------------------------------------------
#
# Project created by QtCreator 2017-04-08T14:14:36
#
#-------------------------------------------------

QT       += core gui widgets testlib

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

HEADERS += RequestParserTests.h

HEADERS += \
    HttpMessageParserTests.h

SOURCES += \
    main.cpp \
    HttpMessageParserTests.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../core/release/ -lcore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../core/debug/ -lcore
else:unix: LIBS += -L$$OUT_PWD/../core/ -lcore

INCLUDEPATH += $$PWD/../core $$PWD/../include
DEPENDPATH += $$PWS/../core
