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

#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <memory>
#include <unordered_set>

#include "Connection.h"

class ConnectionManager
{
public:
    ConnectionManager();

    void start(std::shared_ptr<Connection> connection);

    void stop(std::shared_ptr<Connection> connection);

    void stop_all();

private:
    ConnectionManager(const ConnectionManager &) = delete;
    ConnectionManager& operator =(const ConnectionManager &) = delete;

    std::unordered_set<std::shared_ptr<Connection>> connections_;
};

#endif // CONNECTIONMANAGER_H
