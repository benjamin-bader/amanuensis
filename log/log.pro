QT -= core gui
CONFIG -= qt

TARGET = log
TEMPLATE = lib

CONFIG += c++14

INCLUDEPATH += \
    $$PWD/../include \

HEADERS += \
    Log.h

SOURCES += \
    Log.cpp


