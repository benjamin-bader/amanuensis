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

#ifndef PROXYFACTORY_H
#define PROXYFACTORY_H

#pragma once

#include "core/global.h"

#include <QObject>

namespace ama
{

class Proxy;

class A_EXPORT ProxyFactory
{   
public:
    ProxyFactory();

    Proxy* create(const int port, QObject* parent = nullptr);
};

} // ama

#endif // PROXYFACTORY_H
