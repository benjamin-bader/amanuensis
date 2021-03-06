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

#include "Proxy.h"

#if defined(Q_OS_WIN)
#include "win/WindowsProxy.h"
#endif

using namespace ama;

ProxyFactory::ProxyFactory()
{

}

std::shared_ptr<Proxy> ProxyFactory::create(const int port)
{
#if defined(Q_OS_WIN)
    return std::make_shared<ama::win::WindowsProxy>(port);
#else
#warning No platform support implemented for this OS, returning generic proxy.
    return std::make_shared<Proxy>(port);
#endif
}
