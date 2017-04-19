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

mac {
    SUBDIRS += \
        trusty \
        trusty-interface

    trusty.depends = trusty-interface
    app.depends += trusty-interface

    BUNDLEAPP = Amanuensis
    HELPERAPP = com.bendb.amanuensis.Trusty
    HELPERAPP_INFO = trusty-Info.plist
    HELPER_APP_LAUNCHD_INFO = trusty-Launchd.plist

    organizer.depends += app trusty
    organizer.commands += $(MKDIR) $$OUT_PWD/$${BUNDLEAPP}.app/Contents/Library/LaunchServices;
    organizer.commands += $(MKDIR) $$OUT_PWD/$${BUNDLEAPP}.app/Contents/Resources;
    organizer.commands += $(MOVE) $$OUT_PWD/$${HELPERAPP} $$OUT_PWD/$${BUNDLEAPP}.app/Contents/Library/LaunchServices;
    organizer.commands += $(COPY) $$PWD/trusty/$${HELPERAPP_INFO} $$OUT_PWD/$${BUNDLEAPP}.app/Contents/Resources;
    organizer.commands += $(COPY) $$PWD/trusty/$${HELPER_APP_LAUNCHD_INFO} $$OUT_PWD/$${BUNDLEAPP}.app/Contents/Resources;

    include(trusty-constants.pri)

    BUNDLEID = com.bendb.amanuensis.$${BUNDLEAPP}

    codesigner.commands += dsymutil $${OUT_PWD}/$${BUNDLEAPP}.app/Contents/MacOS/$${BUNDLEAPP} -o $${OUT_PWD}/$${BUNDLEAPP}.app.dSYM;
    codesigner.commands += $(COPY_DIR) $${OUT_PWD}/$${BUNDLEAPP}.app.dSYM $${OUT_PWD}/$${BUNDLEAPP}.app/Contents/MacOS/$${BUNDLEAPP}.dSYM;
    codesigner.commands += macdeployqt $${OUT_PWD}/$${BUNDLEAPP}.app -always-overwrite -codesign=$${CERTSHA1};
    codesigner.commands += touch -c $${OUT_PWD}/$${BUNDLEAPP}.app;

    CODESIGN_ALLOCATE_PATH=$$system(xcrun -find codesign_allocate)
    codesigner.commands += export CODESIGN_ALLOCATE=$${CODESIGN_ALLOCATE_PATH};
    codesigner.commands += codesign --force --sign $${CERTSHA1} -r=\'designated => anchor apple generic and identifier \"$${BUNDLEID}\" and ((cert leaf[field.1.2.840.113635.100.6.1.9] exists) or (certificate 1[field.1.2.840.113635.100.6.2.6] exists and certificate leaf[field.1.2.840.113635.100.6.1.13] exists and certificate leaf[subject.OU]=$${CERT_OU}))\' --timestamp=none $$OUT_PWD/$${BUNDLEAPP}.app;

    first.depends = $(first) organizer # codesigner
    export(first.depends)
    export(organizer.commands)
    #export(codesigner.commands)

    QMAKE_EXTRA_TARGETS += first organizer codesigner
}
