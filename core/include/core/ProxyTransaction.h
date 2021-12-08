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

#ifndef PROXYTRANSACTION_H
#define PROXYTRANSACTION_H

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "core/common.h"
#include "core/global.h"

#include "core/HttpMessageParser.h"
#include "core/Request.h"
#include "core/Response.h"
#include "core/Transaction.h"

namespace ama
{

class Conn;
class ConnectionPool;

enum class NotificationState : uint8_t
{
    None = 0,
    RequestHeaders = 1,
    RequestBody = 2,
    RequestComplete = 3,
    ResponseHeaders = 4,
    ResponseBody = 5,
    ResponseComplete = 6,

    TLSTunnel = 7,

    Error = 8,
};

class A_EXPORT ProxyTransaction : public Transaction
                                , public std::enable_shared_from_this<ProxyTransaction>
{
public:
    ProxyTransaction(int id, std::shared_ptr<ConnectionPool> connectionPool, std::shared_ptr<Conn> clientConnection);
    virtual ~ProxyTransaction() = default;

    int id() const override { return id_; }
    TransactionState state() const override { return state_; }
    std::error_code error() const override { return error_; }

    Request& request() override { return request_; }
    Response& response() override { return response_; }

    void begin();

private:
    void read_client_request();
    void open_remote_connection();
    void send_client_request_to_remote();

    void read_remote_response();
    void send_remote_response_to_client();

    void establish_tls_tunnel();
    void send_client_request_via_tunnel();
    void send_server_response_via_tunnel();

    void notify_phase_change(ParsePhase phase);
    void do_notification(NotificationState ns);
    void notify_failure(std::error_code ec);

    void release_connections();

private:
    int id_;
    std::error_code error_;

    std::shared_ptr<Conn> client_;
    std::shared_ptr<Conn> remote_;

    std::shared_ptr<ConnectionPool> connection_pool_;

    TransactionState state_;

    HttpMessageParser parser_;
    std::weak_ptr<ProxyTransaction> parent_;

    std::array<uint8_t, 8192> read_buffer_;
    std::unique_ptr<std::array<uint8_t, 8192>> remote_buffer_;

    // Records the exact bytes received from client/server, so that
    // they can be relayed as-is.
    std::vector<uint8_t> raw_input_;

    ParsePhase request_parse_phase_;
    Request request_;

    ParsePhase response_parse_phase_;
    Response response_;

    NotificationState notification_state_;
};

} // namespace ama

#endif // PROXYTRANSACTION_H
