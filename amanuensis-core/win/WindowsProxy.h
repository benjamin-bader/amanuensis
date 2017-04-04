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

#ifndef WINDOWSPROXY_H
#define WINDOWSPROXY_H

#include "global.h"

#include "Proxy.h"

#include <windows.h>
#include <wininet.h>

class A_EXPORT WindowsProxy : public Proxy
{
public:
    WindowsProxy(const int port = 9999);
    virtual ~WindowsProxy();

private:
    INTERNET_PER_CONN_OPTION_LIST originalOptionList;
    INTERNET_PER_CONN_OPTION originalOptions[5];
};

#endif // WINDOWSPROXY_H
