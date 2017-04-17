QT += core
QT -= gui

TARGET = com.bendb.amanuensis.trusty

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

QMAKE_LFLAGS += -F /System/Library/Frameworks/Security.framework/ -sectcreate __TEXT __info_plist $$PWD/trusty-info.plist -sectcreate __TEXT __launchd_plist $$PWD/trusty-launchd.plist

LIBS += -framework Security -framework Cocoa

DESTDIR = ../

SOURCES += main.cpp

DISTFILES += \
    trusty-info.plist \
    trusty-launchd.plist

HEADERS += \
    ../app/mac/TrustyCommon.h

include($$PWD/../mac-common.pri)
