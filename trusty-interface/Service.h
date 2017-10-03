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

#ifndef ISERVICE_H
#define ISERVICE_H

#include <memory>
#include <string>

namespace ama {
namespace trusty {


/*!
 *
 */
class IService
{
public:
    virtual ~IService() {}

    virtual const std::string get_http_proxy_host() = 0;
    virtual int get_http_proxy_port() = 0;

    virtual void set_http_proxy_host(const std::string &host) = 0;
    virtual void set_http_proxy_port(int port) = 0;

    virtual void reset_proxy_settings() = 0;
};

/*! Creates an IService implementation, and connects it to
 *  the server listening on the given UNIX socket @p path.
 *
 * @param path an absolute path to a UNIX socket.
 * @return a unique pointer to the connected IService.
 * @throws throws an exception if creating or connecting fails.
 */
std::unique_ptr<IService> create_client(const std::string &path);

} // namespace trusty
} // namespace ama

#endif // ISERVICE_H
