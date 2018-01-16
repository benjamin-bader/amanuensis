QT -= core gui
CONFIG -= qt

TARGET = trusty-interface
TEMPLATE = lib

CONFIG += staticlib c++14

DEFINES += TRUSTY_INTERFACE_LIBRARY SPDLOG_ENABLE_SYSLOG

INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../log \

include(../trusty-constants.pri)

HEADERS += \
    TrustyCommon.h \
    MessageProcessor.h \
    Service.h \
    CFRef.h \
    ProxyState.h \
    UnixSocket.h \
    Bytes.h \
    OsLogSink.h \
    TLog.h

SOURCES += \
    MessageProcessor.cpp \
    Service.cpp \
    TrustyCommon.cpp \
    ProxyState.cpp \
    UnixSocket.cpp \
    OsLogSink.cpp \
    TLog.cpp
