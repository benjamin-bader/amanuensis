QT -= core gui
CONFIG -= qt

TARGET = trusty-interface
TEMPLATE = lib

CONFIG += staticlib c++14

DEFINES += TRUSTY_INTERFACE_LIBRARY

include(../trusty-constants.pri)

HEADERS += \
    TrustyCommon.h \
    MessageProcessor.h \
    Service.h \
    CFRef.h

SOURCES += \
    MessageProcessor.cpp \
    Service.cpp \
    TrustyCommon.cpp
