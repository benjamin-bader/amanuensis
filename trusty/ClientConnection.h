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

#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

#include <memory>

namespace ama { namespace trusty {

class IService;

class ClientConnection
{
public:
    ClientConnection(IService *service, int client_fd);
    ~ClientConnection();

    void handle();

private:
    class impl;
    std::unique_ptr<impl> impl_;
};

}} // ama::trusty

#endif // CLIENTCONNECTION_H
