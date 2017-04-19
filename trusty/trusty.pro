QT =
CONFIG -= qt

TARGET = com.bendb.amanuensis.trusty

CONFIG += console c++14
CONFIG -= app_bundle

TEMPLATE = app

include($$PWD/../trusty-constants.pri)
include($$PWD/../trusty-libs.pri)

QMAKE_LFLAGS += \
    -sectcreate __TEXT __info_plist $$PWD/trusty-info.plist \
    -sectcreate __TEXT __launchd_plist $$PWD/trusty-launchd.plist

LIBS += -framework Security -framework ServiceManagement

LIBS += -L$${OUT_PWD}/../trusty-interface/ -ltrusty-interface

DESTDIR = ../

SOURCES += main.cpp \
    TrustyServer.cpp \
    OSLoggable.cpp

DISTFILES += \
    trusty-info.plist \
    trusty-launchd.plist

HEADERS += \
    ../TrustyCommon.h \
    TrustyServer.h \
    OSLoggable.h

