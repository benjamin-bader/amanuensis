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

#pragma comment(lib, "wininet")

#include "win/WindowsProxy.h"

#include "log/Log.h"

namespace ama {

namespace {

constexpr const bool is_string_value(DWORD optionValue)
{
    return optionValue == INTERNET_PER_CONN_AUTOCONFIG_URL
            || optionValue == INTERNET_PER_CONN_PROXY_BYPASS
            || optionValue == INTERNET_PER_CONN_PROXY_SERVER;
}

} // namespace

WindowsProxy::WindowsProxy(int port, QObject* parent) :
    Proxy(port, parent)
{    
    DWORD optionListSize = sizeof(INTERNET_PER_CONN_OPTION_LIST);

    // First, query the system's current proxy settings and save them
    originalOptions[0].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
    originalOptions[1].dwOption = INTERNET_PER_CONN_AUTODISCOVERY_FLAGS;
    originalOptions[2].dwOption = INTERNET_PER_CONN_FLAGS;
    originalOptions[3].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
    originalOptions[4].dwOption = INTERNET_PER_CONN_PROXY_SERVER;

    originalOptionList.dwOptionCount = static_cast<DWORD>(originalOptions.size());
    originalOptionList.dwOptionError = 0;
    originalOptionList.pszConnection = NULL;
    originalOptionList.dwSize = optionListSize;
    originalOptionList.pOptions = &originalOptions[0];

    if (! InternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &originalOptionList, &optionListSize))
    {
        log::error("Could not query current proxy settings", log::LastErrorValue{});
        throw std::domain_error("Could not query current proxy settings");
    }

    // TODO(ben): Save these settings somewhere?

    // Next, apply our new settings
    const wchar_t proxyName[] = L"127.0.0.1:9999";

    INTERNET_PER_CONN_OPTION_LIST optionList;
    INTERNET_PER_CONN_OPTION options[4];

    options[0].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
    options[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    options[2].dwOption = INTERNET_PER_CONN_FLAGS;
    options[3].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;

    options[0].Value.pszValue = NULL;
    options[1].Value.pszValue = (LPWSTR) proxyName;
    options[2].Value.dwValue  = PROXY_TYPE_PROXY;
    options[3].Value.pszValue = (LPWSTR) L"";

    optionList.dwOptionCount = 4;
    optionList.dwOptionError = 0;
    optionList.pszConnection = NULL;
    optionList.dwSize = sizeof(INTERNET_PER_CONN_OPTION_LIST);
    optionList.pOptions = options;

    if (! InternetSetOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &optionList, sizeof(INTERNET_PER_CONN_OPTION_LIST)))
    {
        log::error("Could not set proxy options", log::LastErrorValue{});
        throw std::domain_error("Could not set proxy options");
    }

    InternetSetOption(NULL, INTERNET_OPTION_PROXY_SETTINGS_CHANGED, NULL, 0);
    InternetSetOption(NULL, INTERNET_OPTION_REFRESH, NULL, 0);
}

WindowsProxy::~WindowsProxy()
{
    unsigned long nSize = sizeof(INTERNET_PER_CONN_OPTION_LIST);
    if (! InternetSetOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &originalOptionList, nSize))
    {
        // wtf
    }

    InternetSetOption(NULL, INTERNET_OPTION_PROXY_SETTINGS_CHANGED, NULL, 0);
    InternetSetOption(NULL, INTERNET_OPTION_REFRESH, NULL, 0);

    for (const auto& option : originalOptions)
    {
        if (is_string_value(option.dwOption) && option.Value.pszValue != NULL)
        {
            GlobalFree(option.Value.pszValue);
        }
    }
}

} // ama
