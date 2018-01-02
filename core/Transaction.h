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

#ifndef TRANSACTION_H
#define TRANSACTION_H

#pragma once

#include <memory>
#include <string>
#include <system_error>
#include <utility>

#include "global.h"

#include "Listenable.h"
#include "Request.h"
#include "Response.h"

namespace ama
{

class Transaction;

/**
 * @brief
 * A listener interface that receives events in the lifecycle of a Transaction.
 */
class A_EXPORT TransactionListener
{
public:
    virtual ~TransactionListener() {}

    virtual void on_transaction_start(Transaction &tx) = 0;
    virtual void on_request_read(Transaction &tx) = 0;

    virtual void on_response_headers_read(Transaction &tx) = 0;
    virtual void on_response_read(Transaction &tx) = 0;

    virtual void on_transaction_complete(Transaction &tx) = 0;

    virtual void on_transaction_failed(Transaction &tx) = 0;
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

/**
 * Manually exporting the Listenable specialization, because MSVC can't figure it out on its own.
 */
template class A_EXPORT Listenable<TransactionListener>;

/**
 * @brief
 * A Transaction represents the lifecycle of an HTTP Request/Response pair.
 *
 * It provides access to a series of well-defined events in the process
 * of handling client requests and server responses, as well as its own
 * state and, if applicable, error information.
 *
 * Interested parties may register themselves for request/response events by
 * implementing TransactionListener, and registering themselves with add_listener.
 *
 * It's important to note that Transaction does not store request or response
 * contents, such as the request URL or the response codes.  Such data is emitted
 * in TransactionListener events only - listener implementations are responsible
 * for managing their own storage, as their needs dictate.
 *
 * Transaction instances are created and owned by a Proxy object.
 *
 * @remarks
 * A Transaction is modeled as a state machine, defined by the TransactionState
 * enum.
 */
class A_EXPORT Transaction : public Listenable<TransactionListener>
{
public:
    Transaction() {}

    virtual ~Transaction()
    {
    }

    virtual int id() const = 0;

    virtual TransactionState state() const = 0;

    // Gets the error state of this Transaction.
    //
    // If the TransactionState is not TransactionState::Error,
    // the error code will be 0.  This does not mean that
    // the transaction succeeded!  It merely means that it has
    // not (yet) failed.
    virtual std::error_code error() const = 0;

    virtual Request& request() = 0;
    virtual Response& response() = 0;

private:
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
};

} // namespace ama

#endif // TRANSACTION_H
