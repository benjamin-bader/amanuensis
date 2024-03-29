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

#include "TrustyService.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

#include <SystemConfiguration/SystemConfiguration.h>

#include "trusty/CFLog.h"
#include "trusty/CFRef.h"
#include "trusty/TrustyCommon.h"

namespace ama { namespace trusty {

template <> class is_cfref<SCDynamicStoreRef> : public std::true_type {};
template <> class is_cfref<SCPreferencesRef> : public std::true_type {};
template <> class is_cfref<SCNetworkSetRef> : public std::true_type {};
template <> class is_cfref<SCNetworkServiceRef> : public std::true_type {};
template <> class is_cfref<SCNetworkProtocolRef> : public std::true_type {};

namespace {

using CFIndexValue = log::LogValue<CFIndex>;

/*! The PreferencesLocker class is an RAII wrapper around SCPreferencesLock
 *  and SCPreferencesUnlock.
 *
 *  It does _not_ acquire any ownership interest in its SCPreferencesRef.
 */
class PreferencesLocker
{
public:
    PreferencesLocker(SCPreferencesRef ref, bool wait = false) : ref_(ref), locked_(false)
    {
        if (ref == nullptr)
        {
            throw std::invalid_argument{"null SCPreferencesRef"};
        }

        if (! SCPreferencesLock(ref, wait))
        {
            std::stringstream ss;
            ss << SCErrorString(SCError());
            throw std::runtime_error{ss.str()};
        }

        locked_ = true;
    }

    ~PreferencesLocker() noexcept
    {
        try
        {
            unlock();
        }
        catch (const std::exception& ex)
        {
            log::error("Exception in PreferencesLocker dtor", log::CStrValue("what", ex.what()));
            // ignore exceptions in dtor
        }
    }

    void unlock()
    {
        if (is_locked())
        {
            if (! SCPreferencesUnlock(ref_))
            {
                std::stringstream ss;
                ss << SCErrorString(SCError());
                throw std::runtime_error{ss.str()};
            }
            locked_ = false;
        }
    }

    bool is_locked() const noexcept
    {
        return locked_;
    }

private:
    SCPreferencesRef ref_;
    bool locked_;
};

const CFStringRef kGlobalIPv4Config = CFSTR("State:/Network/Global/IPv4");
const CFStringRef kPrimaryService = CFSTR("PrimaryService");

std::string cfstring_as_std_string(CFStringRef ref)
{
    if (ref == nullptr)
    {
        return {};
    }

    CFIndex length = CFStringGetLength(ref);
    if (length == 0)
    {
        return {};
    }

    CFRange allStringBytes = CFRangeMake(0, length);
    CFIndex outSize;
    CFIndex converted = CFStringGetBytes(
                ref,
                allStringBytes,
                kCFStringEncodingUTF8,
                0,        // lossByte
                false,    // isExternalRepresentation
                nullptr,  // buffer
                0,        // maxBufLen
                &outSize);

    if (converted == 0 || outSize == 0)
    {
        return {};
    }

    size_t num_elements = static_cast<size_t>(outSize);

    std::vector<char> elements(num_elements + 1); // Leave room for the null terminator
    converted = CFStringGetBytes(
                ref,
                allStringBytes,
                kCFStringEncodingUTF8,
                0,        // lossByte
                false,    // isExternalRepresentation
                reinterpret_cast<UInt8*>(elements.data()),
                outSize,
                nullptr); // usedBufLen

    if (converted == 0)
    {
        return {};
    }

    elements[num_elements] = '\0';
    return std::string{elements.data(), num_elements};
}

int32_t cfnumber_as_int32_t(CFNumberRef ref)
{
    if (ref == nullptr)
    {
        return 0;
    }

    int32_t value;
    if (! CFNumberGetValue(ref, kCFNumberSInt32Type, &value))
    {
        //log_warn("CFNumberGetValue returned false for an expected int32_t; value=%d", value);
    }

    return value;
}

bool cfnumber_as_bool(CFNumberRef ref)
{
    if (ref == nullptr)
    {
        return false;
    }

    int32_t value;
    if (! CFNumberGetValue(ref, kCFNumberSInt32Type, &value))
    {
        //log_warn("CFNumberGetValue returned false for an expected int32_t; value=%d", value);
    }

    return value != 0;
}

CFStringRef get_primary_service_id()
{
    CFRef<SCDynamicStoreRef> storeRef = SCDynamicStoreCreate(nullptr, CFSTR("get_current_proxy_state"), nullptr, nullptr);
    CFRef<CFDictionaryRef> ipv4 = (CFDictionaryRef) SCDynamicStoreCopyValue(storeRef, kGlobalIPv4Config);
    CFStringRef primaryServiceId = (CFStringRef) CFDictionaryGetValue(ipv4, kPrimaryService);

    if (primaryServiceId == nullptr)
    {
        throw std::runtime_error{"no primary network service?"};
    }

    CFRetain(primaryServiceId);
    return primaryServiceId;
}

} // anonymous namespace

bool is_valid_hostname_or_empty(std::string_view s)
{
    if (s.empty())
    {
        return true;
    }

    if (s.size() >= 254)
    {
        // https://devblogs.microsoft.com/oldnewthing/20120412-00/?p=7873
        return false;
    }

    if (!std::isalpha(s.at(0)) && !std::isdigit(s.at(0)))
    {
        // Hostnames must start with a letter or a number, and may not start with
        // a hyphen.
        return false;
    }

    if (s.size() >= 4 && s.substr(4) == "xn--")
    {
        // We deliberately aren't supporting punycode.  We never use it so if this hits,
        // someone has breached our attempts at authn and authz.
        return false;
    }

    // This predicate doesn't support Unicode hostnames, but that's OK.
    return std::all_of(
            s.begin(),
            s.end(),
            [](char c) { return std::isalpha(c) || std::isdigit(c) || c == '.' || c == '-'; }
    );
}

ProxyState TrustyService::get_http_proxy_state()
{
    CFRef<SCPreferencesRef> prefs    = SCPreferencesCreate(kCFAllocatorDefault, CFSTR("com.bendb.amanuensis.Trusty"), nullptr);
    CFRef<SCNetworkSetRef>  netset   = SCNetworkSetCopyCurrent(prefs);
    CFRef<CFArrayRef>       services = SCNetworkSetCopyServices(netset);

    CFRef<CFStringRef> primaryServiceId = get_primary_service_id();
    CFIndex numServices = CFArrayGetCount(services);
    for (CFIndex i = 0; i < numServices; ++i)
    {
        SCNetworkServiceRef service = (SCNetworkServiceRef) CFArrayGetValueAtIndex(services, i);

        CFStringRef serviceId = SCNetworkServiceGetServiceID(service);
        if (! CFEqual(primaryServiceId, serviceId))
        {
            continue;
        }

        CFRef<SCNetworkProtocolRef> proxies = SCNetworkServiceCopyProtocol(service, kSCNetworkProtocolTypeProxies);
        if (proxies == nullptr)
        {
            log::warn("Proxy protocol does not exist for primary network service", log::CFStringValue("id", serviceId));
            continue;
        }

        if (! SCNetworkProtocolGetEnabled(proxies))
        {
            log::warn("Proxy protocol disabled for primary network service", log::CFStringValue("id", serviceId));
            continue;
        }

        std::string host{};
        int32_t port = 0;
        bool enabled = false;

        CFDictionaryRef config = SCNetworkProtocolGetConfiguration(proxies);
        if (config)
        {
            CFStringRef httpProxyHost = (CFStringRef) CFDictionaryGetValue(config, kSCPropNetProxiesHTTPProxy);
            CFNumberRef httpProxyPort = (CFNumberRef) CFDictionaryGetValue(config, kSCPropNetProxiesHTTPPort);
            CFNumberRef enabledRef    = (CFNumberRef) CFDictionaryGetValue(config, kSCPropNetProxiesHTTPEnable);

            host    = (httpProxyHost != nullptr) ? cfstring_as_std_string(httpProxyHost) : "";
            port    = (httpProxyPort != nullptr) ? cfnumber_as_int32_t(httpProxyPort) : 0;
            enabled = (enabledRef    != nullptr) ? cfnumber_as_bool(enabledRef) : false;
        }

        return { enabled, host, port };
    }

    log::error(
        "No proxy-aware network service defined for the current NetworkSet",
        log::CFStringValue("set_name", SCNetworkSetGetName(netset)),
        log::CFStringValue("set_id", SCNetworkSetGetSetID(netset))
    );

    return { false, "", 0 };
}

void TrustyService::set_http_proxy_state(const ProxyState &state)
{
    log::info("Setting proxy state",
              log::BoolValue("enabled", state.is_enabled()),
              log::StringValue("host", state.get_host()),
              log::I32Value("port", state.get_port()));

    CFRef<SCPreferencesRef> prefs = SCPreferencesCreate(kCFAllocatorDefault, CFSTR("com.bendb.amanuensis.Trusty"), nullptr);
    PreferencesLocker locker{prefs, /* wait */ true};

    CFRef<SCNetworkSetRef> netset = SCNetworkSetCopyCurrent(prefs);

    CFRef<CFArrayRef> services = SCNetworkSetCopyServices(netset);

    bool didUpdate = false;
    CFRef<CFStringRef> primaryServiceId = get_primary_service_id();
    CFIndex numServices = CFArrayGetCount(services);

    for (CFIndex i = 0; i < numServices; ++i)
    {
        SCNetworkServiceRef service = (SCNetworkServiceRef) CFArrayGetValueAtIndex(services, i);

        log::verbose("Examining one network service", CFIndexValue("index", i));

        CFStringRef serviceId = SCNetworkServiceGetServiceID(service);
        if (! CFEqual(primaryServiceId, serviceId))
        {
            continue;
        }

        CFRef<SCNetworkProtocolRef> proxies = SCNetworkServiceCopyProtocol(service, kSCNetworkProtocolTypeProxies);
        if (proxies == nullptr)
        {
            log::warn("Proxy protocol does not exist for primary network service", log::CFStringValue("id", serviceId));
            continue;
        }

        if (! SCNetworkProtocolGetEnabled(proxies))
        {
            log::warn("Proxy protocol disabled for primary network service", log::CFStringValue("id", serviceId));
            continue;
        }

        CFDictionaryRef config = SCNetworkProtocolGetConfiguration(proxies);
        CFRef<CFMutableDictionaryRef> copy = nullptr;

        if (!state.get_host().empty() && config != nullptr)
        {
            copy = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, config);
        }
        else if (!state.get_host().empty())
        {
            copy = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        }
        else
        {
            // ProxyState has an empty hostname, so we'll just remove the entire dictionary.
        }

        if (copy != nullptr)
        {
            int32_t enabled = state.is_enabled() ? 1 : 0;
            int32_t port = state.get_port();
            CFRef<CFStringRef> hostRef = CFStringCreateWithCString(kCFAllocatorDefault, state.get_host().c_str(), kCFStringEncodingUTF8);
            CFRef<CFNumberRef> portRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &port);
            CFRef<CFNumberRef> enabledRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &enabled);

            CFDictionarySetValue(copy, kSCPropNetProxiesHTTPProxy, hostRef);
            CFDictionarySetValue(copy, kSCPropNetProxiesHTTPPort, portRef);
            CFDictionarySetValue(copy, kSCPropNetProxiesHTTPEnable, enabledRef);
        }

        if (SCNetworkProtocolSetConfiguration(proxies, copy))
        {
            didUpdate = true;
        }
        else
        {
            log::error(
                "Error saving proxy protocol configuration",
                log::CFStringValue("service_id", serviceId),
                log::CStrValue("error", SCErrorString(SCError()))
            );
        }

        break;
    }

    if (didUpdate)
    {
        if (! SCPreferencesCommitChanges(prefs))
        {
            log::error("SCPreferencesCommitChanges failed", log::CStrValue("error", SCErrorString(SCError())));
        }

        if (! SCPreferencesApplyChanges(prefs))
        {
            log::error("SCPreferencesApplyChanges failed", log::CStrValue("error", SCErrorString(SCError())));
        }
    }
    else
    {
        log::warn("Active network set did not contain any proxy-enabled service");
    }

    locker.unlock();
}

void TrustyService::reset_proxy_settings()
{
    log::info("TrustyService::reset_proxy_settings()");
}

uint32_t TrustyService::get_current_version()
{
    return ama::kToolVersion;
}

}} // ama::trusty
