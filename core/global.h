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

#ifndef CORE_GLOBAL_H
#define CORE_GLOBAL_H

#pragma once

#include <QtCore/qglobal.h>

#if defined(CORE_LIBRARY)
#  define A_EXPORT Q_DECL_EXPORT
#  define A_EXPORT_ONLY Q_DECL_EXPORT
#else
#  define A_EXPORT Q_DECL_IMPORT
#  define A_EXPORT_ONLY
#endif

#endif // CORE_GLOBAL_H
