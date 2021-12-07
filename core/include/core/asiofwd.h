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

#ifndef ASIOFWD_H
#define ASIOFWD_H

#pragma once

// Useful forward-declarations for keeping ASIO out of headers.

namespace asio {

class io_context;
class io_service;

template <typename Protocol>
class stream_socket_service;

template <typename Protocol, typename StreamSocketService>
class basic_stream_socket;

template <typename InternetProtocol>
class basic_endpoint;

namespace ip {

template <typename Protocol>
class basic_socket;

template <typename InternetProtocol>
class resolver_service;

template <typename Protocol, typename ResolverService>
class basic_resolver;

class tcp;

} // namespace ip

} // namespace asio

#endif // ASIOFWD_H
