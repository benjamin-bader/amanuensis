QT =
CONFIG -= qt

TARGET = com.bendb.amanuensis.Trusty

CONFIG += console c++14
CONFIG -= app_bundle

TEMPLATE = app

include($$PWD/../trusty-constants.pri)
include($$PWD/../trusty-libs.pri)

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

SOURCES += main.cpp

DISTFILES += \
    trusty-info.plist \
    trusty-launchd.plist

HEADERS += \
    ../TrustyCommon.h

DISTFILES += \
    trusty-info.plist \
    trusty-launchd.plist

INFO_PLIST_PATH = $$shell_quote($${DESTDIR}$${TARGET}.app/Contents/Info.plist)

HELPER_IDENTIFIER = com.bendb.amanuensis.Trusty
APP_IDENTIFIER = com.bendb.amanuensis.Amanuensis

# This needs to be pre-linking because this plist is embedded directly into the Trusty binary in an __info_plist section.
QMAKE_PRE_LINK += /usr/libexec/PlistBuddy -c \'Set :SMAuthorizedClients:0 'identifier \\\"$${APP_IDENTIFIER}\\\" and certificate leaf = H\\\"$${CERTSHA1}\\\"'\' $$PWD/trusty-info.plist

QMAKE_CFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_OBJECTIVE_CFLAGS_RELEASE =  $$QMAKE_OBJECTIVE_CFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO

codesigner.commands += dsymutil $${DESTDIR}$${TARGET} -o $${DESTDIR}$${TARGET}.dSYM;
CODESIGN_ALLOCATE_PATH=$$system(xcrun -find codesign_allocate)
codesigner.commands += export CODESIGN_ALLOCATE=$${CODESIGN_ALLOCATE_PATH};
codesigner.commands += codesign --force --sign $${CERTSHA1} -r=\'designated => identifier \"$${TARGET}\" and certificate leaf = H\"$${CERTSHA1}\"\' --timestamp=none $${DESTDIR}$${TARGET};

first.depends = $(first) codesigner
export(first.depends)
export(codesigner.commands)

QMAKE_EXTRA_TARGETS += first codesigner

