# Amanuensis - Web Traffic Inspector
#
# Copyright (C) 2017 Benjamin Bader
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app
TARGET = Amanuensis

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

SOURCES += \
    main.cpp \
    MainWindow.cpp \
    LogSetup.cpp \

HEADERS  += \
    MainWindow.h \
    LogSetup.h \

FORMS    += MainWindow.ui

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../core/release/ -lcore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../core/debug/ -lcore
else:unix: LIBS += -L$$OUT_PWD/../core/ -lcore

INCLUDEPATH += $$PWD/../core $$PWD/../include
DEPENDPATH += $$PWS/../core

DEFINES += ASIO_STANDALONE ASIO_HAS_STD_CHRONO ASIO_HAS_MOVE

windows {
    HEADERS += \
        win/WindowsLogSetup.h \

    SOURCES += \
        win/WindowsLogSetup.cpp \
}

macx {
    QMAKE_TARGET_BUNDLE_PREFIX=com.bendb.amanuensis

    QMAKE_CXXFLAGS += \
        -Wno-unused-local-typedef \ # ASIO has unused typedefs, which is unfortunate.
        -isystem $$PWD/../include/

    include($$PWD/../trusty-constants.pri)
    include($$PWD/../trusty-libs.pri)

    LIBS += -L$${OUT_PWD}/../trusty-interface/ -ltrusty-interface
    INCLUDEPATH += $$PWD/../trusty-interface

    HEADERS += \
        mac/MacProxy.h \
        mac/MacLogSetup.h \

    SOURCES += \
        mac/MacProxy.cpp \
        mac/MacLogSetup.cpp \

    DISTFILES += \
        Info.plist

    APP_PATH = $$shell_quote($${DESTDIR}$${TARGET}.app)

    INFO_PLIST_PATH = $$shell_quote($$APP_PATH/Contents/Info.plist)

    HELPER_IDENTIFIER = com.bendb.amanuensis.Trusty

    plist.commands += $(COPY) $$PWD/Info.plist $${INFO_PLIST_PATH};
    plist.commands += /usr/libexec/PlistBuddy -c \"Set :CFBundleIdentifier com.bendb.amanuensis.$${TARGET}\" $${INFO_PLIST_PATH};
    plist.commands += /usr/libexec/PlistBuddy -c \'Set :SMPrivilegedExecutables:$${HELPER_IDENTIFIER} 'identifier \\\"$${HELPER_IDENTIFIER}\\\" and certificate leaf = H\\\"$${CERTSHA1}\\\"'\' $${INFO_PLIST_PATH};

    first.depends = $(first) plist

    export(first.depends)
    export(plist.depends)

    QMAKE_EXTRA_TARGETS += first plist
}
