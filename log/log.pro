QT -= gui

TARGET = log
TEMPLATE = lib

CONFIG += c++14

DEFINES += LOG_LIBRARY

INCLUDEPATH += \
    $$PWD/../include \

HEADERS += \
    Log.h \
    StringStreamLogValueVisitor.h

SOURCES += \
    Log.cpp


