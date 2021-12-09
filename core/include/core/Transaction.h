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

#include "core/ConnectionPool.h"
#include "core/HttpMessageParser.h"
#include "core/Request.h"
#include "core/Response.h"

#include <QObject>

#include <array>
#include <cstdint>
#include <memory>
#include <system_error>
#include <vector>

namespace ama {

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

/**
 * @brief Models the Transaction class' state machine.
 *
 * @dot
 * digraph TransactionState {
 *   node [shape=record, fontname=Helvetica, fontsize=10];
 *   start [ label="<start>"];
 *   RequestLine;
 *   RequestHeaders;
 *   RequestBody;
 *   ResponseStatus;
 *   ResponseHeaders;
 *   ResponseBody;
 *   Complete;
 *   Error;
 *
 *   start -> RequestLine;
 *   RequestLine -> RequestHeaders;
 *   RequestHeaders -> RequestBody;
 *   RequestHeaders -> ResponseStatus;
 *   RequestBody -> ResponseStatus;
 *   ResponseStatus -> ResponseHeaders;
 *   ResponseHeaders -> ResponseBody;
 *   ResponseHeaders -> Complete;
 *   ResponseBody -> Complete;
 *
 *   RequestLine -> Error;
 *   RequestHeaders -> Error;
 *   RequestBody -> Error;
 *   ResponseStatus -> Error;
 *   ResponseHeaders -> Error;
 *   ResponseBody -> Error;
 * }
 * @enddot
 */
enum A_EXPORT TransactionState
{
    // No data has been received yet.
    Start = 0,

    // Reading the first line of the client request.
    RequestLine,

    // Reading the client request headers, if any.
    RequestHeaders,

    // Reading the client request body, if any.
    RequestBody,

    // Reading the first line of the server response.
    ResponseStatus,

    // Reading the server response's headers, if any.
    ResponseHeaders,

    // Reading the server response's body, if any.
    ResponseBody,

    // The transation has finished - the client request was
    // received and relayed to the server, which responded
    // comprehensibly.
    //
    // This does not mean that the HTTP request was successful -
    // the server may have responded with an error code, for
    // example - but that is outside of our purview here.
    Complete,

    // The proxy transaction failed.
    Error = 0xFFFF
};

class A_EXPORT Transaction : public QObject
{
    Q_OBJECT

public:
    Transaction(int id, ConnectionPool* connectionPool, const std::shared_ptr<Conn>& clientConnection, QObject* parent = nullptr);
    virtual ~Transaction() = default;

    int id() const;
    TransactionState state() const;
    Request& request();
    Response& response();

public slots:
    void begin();

signals:
    void on_transaction_start(ama::Transaction *tx);
    void on_request_read(ama::Transaction *tx);
    void on_response_headers_read(ama::Transaction *tx);
    void on_response_read(ama::Transaction *tx);
    void on_transaction_complete(ama::Transaction *tx);
    void on_transaction_failed(ama::Transaction *tx);

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

    std::shared_ptr<Conn> client_;
    std::shared_ptr<Conn> remote_;

    ConnectionPool* connection_pool_;

    TransactionState state_;

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
};

}
