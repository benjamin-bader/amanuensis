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

#include "ProxyFactory.h"

#include <QtGlobal>

#if defined(Q_OS_WIN)
#include "win/WindowsProxy.h"
#elif defined(Q_OS_MACOS)
#include "mac/MacProxy.h"
#else
#include "core/Proxy.h"
#endif

using namespace ama;

namespace ProxyFactory {

Proxy* Create(int port, QObject* parent)
{
#if defined(Q_OS_WIN)
    return new WindowsProxy(port, parent);
#elif defined(Q_OS_MACOS)
    return new MacProxy(port, parent);
#else
#warning No platform support implemented for this OS, returning generic proxy.
    return new Proxy(port, parent);
#endif
}

}
