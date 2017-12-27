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

#include <iostream>
#include <memory>
#include <string>
#include <sstream>

#include <SystemConfiguration/SystemConfiguration.h>

#include "CFRef.h"
#include "TrustyCommon.h"

namespace ama { namespace trusty {

template <> class is_cfref<SCDynamicStoreRef> : public std::true_type {};
template <> class is_cfref<SCPreferencesRef> : public std::true_type {};
template <> class is_cfref<SCNetworkSetRef> : public std::true_type {};
template <> class is_cfref<SCNetworkServiceRef> : public std::true_type {};
template <> class is_cfref<SCNetworkProtocolRef> : public std::true_type {};

namespace {

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
            std::cerr << "Exception in dtor: " << ex.what() << std::endl;
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
        std::cerr << "Warning: CFNumberGetValue returned false for an expected int32_t; value=" << value << std::endl;
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
        std::cerr << "Warning: CFNumberGetValue returned false for an expected int32_t; value=" << value << std::endl;
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

        if (! CFEqual(primaryServiceId, SCNetworkServiceGetServiceID(service)))
        {
            continue;
        }

        CFRef<SCNetworkProtocolRef> proxies = SCNetworkServiceCopyProtocol(service, kSCNetworkProtocolTypeProxies);
        if (proxies == nullptr)
        {
            std::cerr << "Proxy protocol does not exist for primary network service" << std::endl;
            continue;
        }

        if (! SCNetworkProtocolGetEnabled(proxies))
        {
            std::cerr << "Proxy protocol disabled for primary network service" << std::endl;
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

    std::cerr << "No proxy-aware network service defined for the current NetworkSet" << std::endl;
    return { false, "", 0 };
}

void TrustyService::set_http_proxy_state(const ProxyState &state)
{
    std::cerr << "TrustyService::set_http_proxy_state("
              << "enabled=" << state.is_enabled()
              << ", host=" << state.get_host()
              << ", port=" << state.get_port()
              << ")" << std::endl;

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

        if (! CFEqual(primaryServiceId, SCNetworkServiceGetServiceID(service)))
        {
            continue;
        }

        CFRef<SCNetworkProtocolRef> proxies = SCNetworkServiceCopyProtocol(service, kSCNetworkProtocolTypeProxies);
        if (proxies == nullptr)
        {
            std::cerr << "Proxy protocol does not exist for primary network service" << std::endl;
            continue;
        }

        if (! SCNetworkProtocolGetEnabled(proxies))
        {
            std::cerr << "Proxy protocol disabled for primary network service" << std::endl;
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
            copy = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, config);
        }
        else
        {
            // ProxyState has an empty hostname, so we'll just remove the entire dictionary.
            //std::cerr << "ProxyState has an empty hostname; clearing settings." << std::endl;
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
            std::cerr << "Error saving proxy protocol configuration: " << SCErrorString(SCError()) << std::endl;
        }

        break;
    }

    if (didUpdate)
    {
        if (! SCPreferencesCommitChanges(prefs))
        {
            std::cerr << "SCPreferencesCommitChanges failed: " << SCErrorString(SCError()) << std::endl;
        }

        if (! SCPreferencesApplyChanges(prefs))
        {
            std::cerr << "SCPreferencesApplyChanges failed: " << SCErrorString(SCError()) << std::endl;
        }
    }
    else
    {
        std::cerr << "Active network set did not contain any proxy-enabled service" << std::endl;
    }

    locker.unlock();
}

void TrustyService::reset_proxy_settings()
{
    std::cerr << "TrustyService::reset_proxy_settings()" << std::endl;
}

uint32_t TrustyService::get_current_version()
{
    return ama::kToolVersion;
}

}} // ama::trusty
