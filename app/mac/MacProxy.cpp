// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2017 Benjamin Bader
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "MacProxy.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>

#include <QDebug>
#include <QSettings>

#include <errno.h>
#include <poll.h>
#include <syslog.h>
#include <sys/socket.h>
#include <unistd.h>

#include <asio.hpp>

#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFError.h>
#include <Security/Security.h>
#include <ServiceManagement/ServiceManagement.h>

#include "TrustyCommon.h"
#include "Service.h"

using namespace ama;

namespace
{
    inline CFStringRef make_cfstring(const std::string &str)
    {
        return CFStringCreateWithCStringNoCopy(NULL, str.c_str(), kCFStringEncodingASCII, NULL);
    }


}

MacProxy::MacProxy(int port) :
    Proxy(port),
    enabled_(false)
{

}

MacProxy::~MacProxy()
{

}

bool MacProxy::is_enabled() const
{
    return enabled_;
}

void MacProxy::enable(std::error_code &ec)
{
    bless_helper_program(ec);
}

void MacProxy::disable(std::error_code &ec)
{
    Q_UNUSED(ec);
}

void MacProxy::bless_helper_program(std::error_code &ec) const
{
    AuthorizationItem authItem = { kSMRightBlessPrivilegedHelper, 0, nullptr, 0 };
    AuthorizationItem authItemArray[] = { authItem };
    AuthorizationRights authRights = { 1, authItemArray };
    AuthorizationRights authRightsArray[] = { authRights };
    AuthorizationFlags authFlags = kAuthorizationFlagDefaults
            | kAuthorizationFlagInteractionAllowed
            | kAuthorizationFlagExtendRights
            | kAuthorizationFlagPreAuthorize;

    AuthorizationRef authRef = nullptr;

    OSStatus status = AuthorizationCreate(authRightsArray, kAuthorizationEmptyEnvironment, authFlags, &authRef);

    if (status == errAuthorizationSuccess)
    {
        CFStringRef helperLabel = make_cfstring(ama::kHelperLabel);

        CFErrorRef error;
        if (! SMJobBless(kSMDomainSystemLaunchd, helperLabel, authRef, &error))
        {
            syslog(LOG_NOTICE, "SMJobBless failed!  %ld", CFErrorGetCode(error));

            ec.assign(CFErrorGetCode(error), std::system_category());
            CFRelease(error);
        }

        AuthorizationFree(authRef, kAuthorizationFlagDefaults);
    }
    else
    {
        syslog(LOG_NOTICE, "AuthorizationCreate failed!");

        ec.assign(static_cast<int>(status), std::system_category());
    }
}

bool MacProxy::get_installed_helper_info(CFDictionaryRef *pRef) const
{
    Q_UNUSED(pRef);
    pRef = nullptr;

    //CFDictionaryRef = SMJobCopyDictionary(kSMDomainSystemLaunchd, CFSTR(kPRIVILEGED_HELPER_LABEL));

    return false;
}

void MacProxy::say_hi()
{
    auto client = ama::trusty::create_client(ama::kHelperSocketPath);

    auto host = client->get_http_proxy_host();
    auto port = client->get_http_proxy_port();

    client->set_http_proxy_host("hello I am an argument");
    client->set_http_proxy_port(9999);
    client->reset_proxy_settings();
}
