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

#include <cstdint>
#include <iostream>
#include <map>
#include <mutex> // for once_flag
#include <string>
#include <vector>

#include <errno.h>
#include <os/log.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QDebug>

#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFError.h>
#include <CoreFoundation/CFString.h>

#include <Security/Security.h>
#include <ServiceManagement/ServiceManagement.h>

#include "Log.h"
#include "TrustyCommon.h"
#include "Service.h"

namespace ama {

namespace {

std::once_flag auth_init_flag;

AuthorizationRef g_auth = nullptr;

inline CFStringRef make_cfstring(const std::string &str)
{
    return CFStringCreateWithCString(kCFAllocatorDefault, str.c_str(), kCFStringEncodingASCII);
}

inline void assert_success(OSStatus status)
{
    if (status != errAuthorizationSuccess)
    {
        throw std::system_error(static_cast<int>(status), std::system_category());
    }
}

void init_auth()
{
    log::log_event(log::Severity::Verbose, "Initializing global auth ref");

    OSStatus status = AuthorizationCreate(NULL, NULL, 0, &g_auth);
    assert_success(status);

    // Next, ensure all of the helper rights are installed.
    std::map<std::string, std::string> auth_names_and_rules = {
        { ama::kClearSettingsRightName,  kAuthorizationRuleAuthenticateAsAdmin },
        { ama::kSetProxyStateRightName,  kAuthorizationRuleAuthenticateAsAdmin },
        { ama::kGetProxyStateRightName,  kAuthorizationRuleClassAllow },
        { ama::kGetToolVersionRightName, kAuthorizationRuleClassAllow },
    };

    for (const auto &pair : auth_names_and_rules)
    {
        // TODO: AuthorizationRightSet for all of these.
        // TODO: If we create these, do we also need to extend them?
        status = AuthorizationRightGet(pair.first.c_str(), nullptr);
        if (status == errAuthorizationDenied)
        {
            CFStringRef rule = CFSTR(kAuthorizationRuleClassAllow);

            status = AuthorizationRightSet(
                        g_auth,
                        pair.first.c_str(),
                        rule,
                        nullptr,
                        nullptr,
                        nullptr); // TODO: string tables for rights descriptions

            CFRelease(rule);
            assert_success(status);
        }
        else
        {
            // If AuthorizationRightGet succeeded, then great, the rule already
            // existed, and there's nothing else to do.
            // If not, then fail.
            assert_success(status);
        }
    }
}

void acquire_rights(std::vector<const char *> &vector)
{
    std::vector<AuthorizationItem> auth_items;
    for (const char *name : vector)
    {
        auth_items.emplace_back(AuthorizationItem{ name, 0, NULL, 0 });
    }

    AuthorizationRights rights = { static_cast<UInt32>(auth_items.size()), auth_items.data() };

    AuthorizationRights *pResultRights;

    OSStatus status = AuthorizationCopyRights(
                g_auth,
                &rights,
                kAuthorizationEmptyEnvironment,
                kAuthorizationFlagDefaults
                    | kAuthorizationFlagExtendRights
                    | kAuthorizationFlagInteractionAllowed
                    | kAuthorizationFlagPreAuthorize,
                &pResultRights);

    for (UInt32 i = 0; i < pResultRights->count; ++i)
    {
        AuthorizationItem item = pResultRights->items[i];
        if (item.flags & kAuthorizationFlagCanNotPreAuthorize)
        {
            log::log_event(log::Severity::Error,
                           "can not preauthorize right",
                           log::CStrValue("right", item.name));
        }
    }

    AuthorizationFreeItemSet(pResultRights);

    assert_success(status);
}

bool should_install_helper_tool()
{
    log::log_event(log::Severity::Info, "Checking helper-tool installation");

    struct stat stat_data;
    int stat_result = ::stat(ama::kHelperSocketPath.c_str(), &stat_data);

    if (stat_result < 0)
    {
        int error_code = errno;
        if (error_code == ENOENT || error_code == ENOTDIR)
        {
            log::log_event(log::Severity::Info, "Helper socket file not found, installation required");
            return true;
        }

        throw std::system_error(error_code, std::system_category());
    }

    //
    if (! S_ISSOCK(stat_data.st_mode))
    {
        log::log_event(log::Severity::Warn, "Helper socket file exists, but is not a socket");
        return true;
    }

    std::vector<const char *> rights { ama::kGetToolVersionRightName };
    acquire_rights(rights);

    AuthorizationExternalForm authExt;
    AuthorizationMakeExternalForm(g_auth, &authExt);

    uint8_t *authExtPtr = (uint8_t *) &authExt;

    std::vector<uint8_t> authBytes(authExtPtr, authExtPtr + sizeof(authExt));

    try
    {
        log::log_event(log::Severity::Info, "Connecting to helper tool");
        auto client = ama::trusty::create_client(ama::kHelperSocketPath, authBytes);
        auto version = client->get_current_version();

        log::log_event(log::Severity::Info,
                       "Helper tool reported its version",
                       log::IntValue("installed_version", version),
                       log::IntValue("current_version", kToolVersion));

        return version != ama::kToolVersion;
    }
    catch (const std::exception &ex)
    {
        // Let's assume, for now, that an exception here means that
        // we can't understand the protocol of the existing tool, and
        // treat connection failures as such - reinstalling won't hurt,
        // and we can do this better, later.
        log::log_event(log::Severity::Warn,
                       "Error connecting to helper",
                       log::CStrValue("ex", ex.what()));
        return true;
    }
}

std::unique_ptr<ama::trusty::IService> create_client_with_rights()
{
    std::vector<const char *> rights = {
        ama::kGetProxyStateRightName,
        ama::kSetProxyStateRightName,
        ama::kClearSettingsRightName,
        ama::kGetToolVersionRightName,
    };

    acquire_rights(rights);

    AuthorizationExternalForm authExt;
    AuthorizationMakeExternalForm(g_auth, &authExt);

    uint8_t *authExtPtr = (uint8_t *) &authExt;

    std::vector<uint8_t> authBytes(authExtPtr, authExtPtr + sizeof(authExt));

    return ama::trusty::create_client(ama::kHelperSocketPath, authBytes);
}

} // anonymous namespace

MacProxy::MacProxy(int port) :
    Proxy(port),
    enabled_(false)
{
    std::call_once(auth_init_flag, init_auth);
}

MacProxy::~MacProxy()
{
    try
    {
        disable();
    }
    catch (const std::exception& ex)
    {
        log::log_event(log::Severity::Error,
                       "Error disabling MacProxy",
                       log::CStrValue("ex", ex.what()));
    }
}

bool MacProxy::is_enabled() const
{
    return enabled_;
}

void MacProxy::enable()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (enabled_)
    {
        return;
    }

    qInfo() << "Checking if tool version is current";

    if (should_install_helper_tool())
    {
        qInfo() << "Tool is missing or outdated; installing anew";
        std::error_code ec;
        for (int attempts = 0; attempts < 3; attempts++)
        {
            ec.clear();
            bless_helper_program(ec);

            if (! ec)
            {
                break;
            }
        }

        if (ec)
        {
            throw std::system_error(ec);
        }
    }

    auto client = create_client_with_rights();

    auto current_state = client->get_http_proxy_state();
    if (! current_state.is_enabled() || current_state.get_host() != "localhost" || current_state.get_port() != port())
    {
        hostname_to_restore_ = current_state.get_host();
        port_to_restore_ = current_state.get_port();

        ama::trusty::ProxyState new_state{ true, "localhost", static_cast<int32_t>(port()) };
        client->set_http_proxy_state(new_state);
    }
    else
    {
        os_log_info(OS_LOG_DEFAULT, "Proxy already enabled, no action needed");
    }

    enabled_ = true;
}

void MacProxy::disable()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (enabled_)
    {
        auto client = create_client_with_rights();
        ama::trusty::ProxyState new_state{false, hostname_to_restore_, port_to_restore_};

        client->set_http_proxy_state(new_state);

        enabled_ = false;
    }
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

    OSStatus status = AuthorizationCopyRights(g_auth, authRightsArray, kAuthorizationEmptyEnvironment, authFlags, NULL);

    if (status == errAuthorizationSuccess)
    {
        CFStringRef helperLabel = make_cfstring(ama::kHelperLabel);

        CFErrorRef error;
        if (! SMJobBless(kSMDomainSystemLaunchd, helperLabel, g_auth, &error))
        {
            CFStringRef desc = CFErrorCopyDescription(error);

            char buffer[256];
            buffer[255] = '\0';
            if (CFStringGetCString(desc, buffer, 255, kCFStringEncodingUTF8))
            {
                log::log_event(log::Severity::Error,
                               "SMJobBless failed!",
                               log::I64Value("code", CFErrorGetCode(error)),
                               log::CStrValue("description", buffer));
            }
            else
            {
                log::log_event(log::Severity::Error,
                               "SMJobBless failed, and so did CFStringGetCString",
                               log::I64Value("code", CFErrorGetCode(error)));
            }

            ec.assign(CFErrorGetCode(error), std::system_category());
            CFRelease(desc);
        }

        if (error)
        {
            CFRelease(error);
        }

        CFRelease(helperLabel);
    }
    else
    {
        log::log_event(log::Severity::Error,
                       "AuthorizationCopyRights failed",
                       log::I32Value("status", status));

        ec.assign(static_cast<int>(status), std::system_category());
    }
}

void MacProxy::say_hi()
{
    auto client = create_client_with_rights();

    ama::trusty::ProxyState state = client->get_http_proxy_state();
    os_log_info(OS_LOG_DEFAULT, "before: host=%{public}s port=%d enabled=%d", state.get_host().c_str(), state.get_port(), state.is_enabled());

    client->set_http_proxy_state(ama::trusty::ProxyState{ true, "localhost", port() });

    state = client->get_http_proxy_state();
    os_log_info(OS_LOG_DEFAULT, "after: host=%{public}s port=%d enabled=%d", state.get_host().c_str(), state.get_port(), state.is_enabled());

    client->reset_proxy_settings();
}

} // ama
