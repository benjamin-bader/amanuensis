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

TEMPLATE = subdirs

INCLUDEPATH += $$PWD/include

SUBDIRS += \
    core \
    app \
    core-test

app.depends = core
core-test.depends = core

macx {
    QMAKE_MAC_SDK = macosx10.13

    SUBDIRS += \
        trusty \
        trusty-interface

    trusty.depends = trusty-interface
    app.depends += trusty-interface

    BUNDLEAPP = Amanuensis
    HELPERAPP = com.bendb.amanuensis.Trusty
    HELPERAPP_INFO = trusty-Info.plist
    HELPER_APP_LAUNCHD_INFO = trusty-Launchd.plist

    BUNDLE_DIR = $$OUT_PWD/app/$${BUNDLEAPP}.app

    QMAKE_EXTRA_VARIABLES += MACDEPLOYQT

    TEMPNAME = $$dirname(QMAKE_QMAKE)
    MACDEPLOYQT = $${TEMPNAME}/macdeployqt

    # 'organizer' will place all build output into an application bundle directory
    organizer.depends += app trusty

    # organizer.commands += rm -rf $${BUNDLE_DIR};

    # Move the built .app from the app subdir to the root
    # organizer.commands += $(MOVE) $$OUT_PWD/app/$${BUNDLEAPP}.app $${BUNDLE_DIR};

    # Move the core lib to the bundle
    organizer.commands += $(MKDIR) $${BUNDLE_DIR}/Contents/Frameworks;
    organizer.commands += $(MOVE) $$OUT_PWD/core/libcore.1.0.0.dylib $${BUNDLE_DIR}/Contents/Frameworks;

    # Set up the trusted-helper files in the bundle
    organizer.commands += $(MKDIR) $${BUNDLE_DIR}/Contents/Library/LaunchServices;
    organizer.commands += $(MKDIR) $${BUNDLE_DIR}/Contents/Resources;
    organizer.commands += $(MOVE) $$OUT_PWD/trusty/$${HELPERAPP} $${BUNDLE_DIR}/Contents/Library/LaunchServices;
    organizer.commands += $(COPY) $$PWD/trusty/$${HELPERAPP_INFO} $${BUNDLE_DIR}/Contents/Resources;
    organizer.commands += $(COPY) $$PWD/trusty/$${HELPER_APP_LAUNCHD_INFO} $${BUNDLE_DIR}/Contents/Resources;

    include(trusty-constants.pri)

    BUNDLEID = com.bendb.amanuensis.$${BUNDLEAPP}

    # The 'codesigner' target does a few things that are intertwined with codesigning:
    # - debug symbols are generated for the app
    # - the executable is patched to link to libcore in the correct location
    # - macdeployqt is invoked to both copy and sign the QT frameworks
    #
    # After all that, we sign the bundle again.
    codesigner.commands += dsymutil $${BUNDLE_DIR}/Contents/MacOS/$${BUNDLEAPP} -o $${OUT_PWD}/app/$${BUNDLEAPP}.app.dSYM;
    codesigner.commands += $(COPY_DIR) $${BUNDLE_DIR}.dSYM $${BUNDLE_DIR}/Contents/MacOS/$${BUNDLEAPP}.dSYM;
    codesigner.commands += install_name_tool -change libcore.1.dylib @executable_path/../Frameworks/libcore.1.0.0.dylib $${BUNDLE_DIR}/Contents/MacOS/$${BUNDLEAPP};
    codesigner.commands += $(EXPORT_MACDEPLOYQT) $${BUNDLE_DIR} -always-overwrite -codesign=$${CERTSHA1};
    codesigner.commands += touch -c $${BUNDLE_DIR};

    CODESIGN_ALLOCATE_PATH=$$system(xcrun -find codesign_allocate)
    codesigner.commands += export CODESIGN_ALLOCATE=$${CODESIGN_ALLOCATE_PATH};
    codesigner.commands += codesign --verbose --force --sign $${CERTSHA1} -r=\'designated => identifier \"$${BUNDLEID}\" and certificate leaf = H\"$${CERTSHA1}\"\' --timestamp=none $${BUNDLE_DIR} 2>&1;

    first.depends = $(first) organizer codesigner
    export(first.depends)
    export(organizer.commands)
    export(codesigner.commands)

    QMAKE_EXTRA_TARGETS += first organizer codesigner
}
