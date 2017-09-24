QT =
CONFIG -= qt

TARGET = com.bendb.amanuensis.trusty

CONFIG += console c++14
CONFIG -= app_bundle

TEMPLATE = app

include($$PWD/../trusty-constants.pri)
include($$PWD/../trusty-libs.pri)

DEFINES += ASIO_STANDALONE ASIO_HAS_STD_CHRONO ASIO_HAS_MOVE

QMAKE_CXXFLAGS += \
    -isystem $$PWD/../include
    -Wno-unused-local-typedef

INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../trusty-interface

QMAKE_LFLAGS += \
    -F /System/Library/Frameworks/SystemConfiguration.framework/ \
    -sectcreate __TEXT __info_plist $$PWD/trusty-info.plist \
    -sectcreate __TEXT __launchd_plist $$PWD/trusty-launchd.plist

LIBS += \
    -framework SystemConfiguration

LIBS += -L$${OUT_PWD}/../trusty-interface/ -ltrusty-interface

DESTDIR = ../

SOURCES += main.cpp \
    OSLoggable.cpp \
    TrustyService.cpp \
    LocalServer.cpp

DISTFILES += \
    trusty-info.plist \
    trusty-launchd.plist

HEADERS += \
    ../TrustyCommon.h \
    OSLoggable.h \
    TrustyService.h \
    LocalServer.h

