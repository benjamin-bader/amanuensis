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

#include <SystemConfiguration/SystemConfiguration.h>

#include <iostream>
#include <memory>
#include <string>
#include <sstream>

#include "TrustyCommon.h"

namespace ama { namespace trusty {

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

    CFIndex num_unicode_chars = CFStringGetLength(ref);
    CFIndex buffer_len = num_unicode_chars * 3;
    std::unique_ptr<char> buffer = std::make_unique<char>(static_cast<size_t>(buffer_len));

    if (! CFStringGetCString(ref, buffer.get(), buffer_len, kCFStringEncodingUTF8))
    {
        std::cerr << "Failed to convert a CFStringRef to a std::string" << std::endl;
        return {};
    }

    return std::string{buffer.get()};
}

int32_t cfnumber_as_int32_t(CFNumberRef ref)
{
    if (ref == nullptr)
    {
        return 0;
    }

    int32_t value;
    if (! CFNumberGetValue(ref, kCFNumberIntType, &value))
    {
        std::cerr << "Warning: CFNumberGetValue returned false for an expected int32_t; value=" << value << std::endl;
    }

    return value;
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

ProxyState get_current_proxy_state(SCDynamicStoreRef /* storeRef */)
{
    CFRef<SCPreferencesRef> prefs = SCPreferencesCreate(kCFAllocatorDefault, CFSTR("com.bendb.amanuensis.Trusty"), nullptr);

    CFRef<SCNetworkSetRef> netset = SCNetworkSetCopyCurrent(prefs);

    CFRef<CFArrayRef> services = SCNetworkSetCopyServices(netset);

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

        if (proxies != nullptr)
        {
            CFDictionaryRef config = SCNetworkProtocolGetConfiguration(proxies);
            bool enabled = SCNetworkProtocolGetEnabled(proxies);
            CFStringRef httpProxyHost = (CFStringRef) CFDictionaryGetValue(config, kSCPropNetProxiesHTTPProxy);
            CFNumberRef httpProxyPort = (CFNumberRef) CFDictionaryGetValue(config, kSCPropNetProxiesHTTPPort);

            return {enabled, cfstring_as_std_string(httpProxyHost), cfnumber_as_int32_t(httpProxyPort) };
        }
    }

    std::cerr << "No proxy-aware network service defined for the current NetworkSet" << std::endl;
    return {false, "", 0};
}

void set_current_proxy_state(SCDynamicStoreRef /* storeRef */, const ProxyState& state)
{
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

        if (proxies != nullptr)
        {
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

            if (copy != nullptr)
            {
                int32_t port = state.get_port();
                CFRef<CFStringRef> hostRef = CFStringCreateWithCString(kCFAllocatorDefault, state.get_host().c_str(), kCFStringEncodingUTF8);
                CFRef<CFNumberRef> portRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &port);

                CFDictionarySetValue(copy, kSCPropNetProxiesHTTPSProxy, hostRef);
                CFDictionarySetValue(copy, kSCPropNetProxiesHTTPPort, portRef);
            }

            SCNetworkProtocolSetEnabled(proxies, state.is_enabled() && copy != nullptr);
            SCNetworkProtocolSetConfiguration(proxies, copy);

            didUpdate = true;
            break;
        }

        locker.unlock();
    }

    if (didUpdate)
    {
        SCPreferencesCommitChanges(prefs);
        SCPreferencesApplyChanges(prefs);

        // Open and close a dynamic-store session; this is (was?) required for some of the config
        // daemons to pick up on the changes.
        SCDynamicStoreRef wakeUp = SCDynamicStoreCreate(nullptr, CFSTR("com.bendb.amanuensis.trusty.TrustyService.WakeUp"), nullptr, nullptr);
        CFRelease(wakeUp);
    }
    else
    {
        std::cerr << "Active network set did not contain any proxy-enabled service" << std::endl;
    }
}


} // anonymous namespace

TrustyService::TrustyService()
{
    ref_ = SCDynamicStoreCreate(kCFAllocatorDefault, CFSTR("com.bendb.amanuensis.trusty.TrustyService"), nullptr, nullptr);
}

ProxyState TrustyService::get_http_proxy_state()
{
    return get_current_proxy_state(ref_);
}

void TrustyService::set_http_proxy_state(const ProxyState &state)
{
    std::cerr << "TrustyService::set_http_proxy_state("
              << "enabled=" << state.is_enabled()
              << ", host=" << state.get_host()
              << ", port=" << state.get_port()
              << ")" << std::endl;

    set_current_proxy_state(ref_, state);
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
