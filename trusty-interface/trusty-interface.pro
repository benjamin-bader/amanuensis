QT -= core gui
CONFIG -= qt

TARGET = trusty-interface
TEMPLATE = lib

CONFIG += staticlib c++14

DEFINES += TRUSTY_INTERFACE_LIBRARY

include(../trusty-constants.pri)
include(../trusty-libs.pri)

HEADERS += \
    Command.h \
    TrustyCommon.h

SOURCES += \
    Command.cpp

