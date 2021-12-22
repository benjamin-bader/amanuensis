// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2021 Benjamin Bader
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

#pragma once

#include "AuthorizedService.h"

#include "trusty/IMessage.h"
#include "trusty/IService.h"

#include <atomic>
#include <memory>
#include <unordered_map>

#include <xpc/xpc.h>

namespace ama::trusty {

class XpcServer : public std::enable_shared_from_this<XpcServer>
{
public:
    explicit XpcServer(const std::shared_ptr<IService>& service, const char* machServiceName);
    virtual ~XpcServer();

    void serve();

private:
    void handleIncomingConnection(xpc_connection_t connection);
    bool isClientSignatureValid(xpc_connection_t connection);

    void handleIncomingMessage(xpc_object_t message);
    bool isMessageSignatureValid(xpc_object_t message);

    std::unique_ptr<IMessage> handleOne(const std::shared_ptr<AuthorizedService>& svc, std::unique_ptr<IMessage>&& message);

private:
    XpcServer(const XpcServer&) = delete;
    XpcServer& operator=(const XpcServer&) = delete;

    std::uint64_t getConnectionId(xpc_connection_t) const;

    friend void CleanUpConnection(void*);

private:
    std::shared_ptr<IService> m_service;
    const char* m_machServiceName;
    xpc_connection_t m_listener;

    std::atomic_uint64_t m_nextId;
    std::unordered_map<std::uint64_t, std::shared_ptr<AuthorizedService>> m_connections;
};

} // ama::trusty
