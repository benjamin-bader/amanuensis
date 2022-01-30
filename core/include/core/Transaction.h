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

#include "core/global.h"
#include "core/ConnectionPool.h"
#include "core/HttpMessageParser.h"
#include "core/Request.h"
#include "core/Response.h"

#include <QEnableSharedFromThis>
#include <QObject>
#include <QSharedPointer>
#include <QTextStream>

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <system_error>
#include <vector>

namespace ama {

enum class A_EXPORT NotificationState
{
    None,
    RequestLine,
    RequestHeaders,
    RequestBody,
    RequestComplete,
    ResponseHeaders,
    ResponseBody,
    ResponseComplete,

    TLSTunnel,

    Error,
};

class A_EXPORT Transaction : public QObject, public QEnableSharedFromThis<Transaction>
{
    Q_OBJECT

public:
    Transaction(int id, ConnectionPool* connectionPool, const std::shared_ptr<IConnection>& clientConnection, QObject* parent = nullptr);
    virtual ~Transaction() = default;

    int id() const;
    NotificationState state() const;
    Request& request();
    Response& response();
    std::error_code error() const;

public slots:
    void begin();

signals:
    void on_transaction_start(const QSharedPointer<ama::Transaction>& tx);
    void on_request_read(const QSharedPointer<ama::Transaction>& tx);
    void on_response_headers_read(const QSharedPointer<ama::Transaction>& tx);
    void on_response_read(const QSharedPointer<ama::Transaction>& tx);
    void on_transaction_complete(const QSharedPointer<ama::Transaction>& tx);
    void on_transaction_failed(const QSharedPointer<ama::Transaction>& tx);

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

    void complete_transaction();

    void release_connections();

private:
    int id_;
    std::error_code error_;

    std::shared_ptr<IConnection> client_;
    std::shared_ptr<IConnection> remote_;

    ConnectionPool* connection_pool_;

    HttpMessageParser parser_;

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

    // Used to guard against double-releasing connections, which is
    // possible in TLS tunneling.  There's a race condition when one end
    // closes the connection - two threads might both try to shut delete
    // connections, which (at least on MSVC in debug builds) can cause
    // a segfault.
    //
    // See https://github.com/benjamin-bader/amanuensis/issues/45 for deets.
    std::atomic_bool is_open_;
};

} // namespace ama

QTextStream& operator<<(QTextStream& out, ama::NotificationState state);
