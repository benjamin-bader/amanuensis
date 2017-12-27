#-------------------------------------------------
#
# Project created by QtCreator 2017-04-08T14:14:36
#
#-------------------------------------------------

QT       += core testlib

TARGET = trusty-test
CONFIG   += console testcase
CONFIG   -= app_bundle

TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++14

DEFINES += ASIO_STANDALONE ASIO_HAS_STD_CHRONO ASIO_HAS_MOVE

HEADERS += \
    ProxyStateTest.h \
    MessageProcessorTest.h \
    BytesTest.h

SOURCES += \
    main.cpp \
    ProxyStateTest.cpp \
    MessageProcessorTest.cpp \
    BytesTest.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

LIBS += -L$$OUT_PWD/../trusty-interface/ -ltrusty-interface

INCLUDEPATH += \
    $$PWD/../trusty-interface \
    $$PWD/../trusty \
    $$PWD/../include \

DEPENDPATH += \
    $$PWS/../trusty \
