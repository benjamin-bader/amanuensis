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

#include "ProxyTransaction.h"

#include <ctime>

#include <chrono>
#include <iomanip>
#include <locale>
#include <sstream>

#include "common.h"
#include "date.h"

#include "HttpMessageParser.h"

using namespace ama;

class ProxyTransaction::impl : public HttpMessageParserListener
                             , public std::enable_shared_from_this<ProxyTransaction::impl>
{
public:
    impl(int id, std::shared_ptr<ConnectionPool> connectionPool, std::shared_ptr<Conn> clientConnection, ProxyTransaction *parent);

    int id() const { return id_; }
    TransactionState state() const { return state_; }
    std::error_code error() const { return error_; }

    void begin();

public:
    virtual void request_method_read(const std::string &method) override;
    virtual void request_uri_read(const std::string &uri) override;

    virtual void response_code_read(int code) override;
    virtual void response_message_read(const std::string &message) override;

    virtual void header_read(const std::string &name, const std::string &value) override;

    virtual void body_chunk_read(const std::string &buffer, size_t len) override;

private:
    void notify_failure();

private:
    int id_;
    std::error_code error_;

    std::shared_ptr<Conn> client_;
    std::shared_ptr<Conn> remote_;

    std::shared_ptr<ConnectionPool> connection_pool_;

    TransactionState state_;

    HttpMessageParser parser_;
    ProxyTransaction *parent_;

    // "temporaries" to hold values that are parsed
    // until we can pass them along to listeners.
    std::string buffer_;
    int response_code_;
};

ProxyTransaction::impl::impl(
        int id,
        std::shared_ptr<ConnectionPool> connectionPool,
        std::shared_ptr<Conn> clientConnection,
        ProxyTransaction *parent)
    : id_(id)
    , error_()
    , client_(clientConnection)
    , remote_(nullptr)
    , connection_pool_(connectionPool)
    , state_(TransactionState::Start)
    , parser_(shared_from_this())
    , parent_(parent)
{
}

void ProxyTransaction::impl::begin()
{
    // TODO
}

#define ASSERT_STATE(expected) do { \
    if (state_ != (expected)) { \
        notify_failure(); \
        return; \
    } \
} while (0)

void ProxyTransaction::impl::request_method_read(const std::string &method)
{
    ASSERT_STATE(TransactionState::RequestLine);
    buffer_ = method;
}

void ProxyTransaction::impl::request_uri_read(const std::string &uri)
{
    ASSERT_STATE(TransactionState::RequestLine);
    parent_->notify_listeners([this, &uri](auto &listener)
    {
        listener->request_line_read(*parent_, buffer_, uri);
    });

    buffer_.clear();
    state_ = TransactionState::RequestHeaders;
}

void ProxyTransaction::impl::response_code_read(int code)
{
    ASSERT_STATE(TransactionState::ResponseStatus);
    response_code_ = code;
}

void ProxyTransaction::impl::response_message_read(const std::string &message)
{
    ASSERT_STATE(TransactionState::ResponseStatus);
    parent_->notify_listeners([this, &message](auto &listener)
    {
        listener->response_status_read(*parent_, response_code_, message);
    });
    state_ = TransactionState::ResponseHeaders;
}

void ProxyTransaction::impl::header_read(const std::string &name, const std::string &value)
{
    switch (state_)
    {
    case TransactionState::RequestHeaders:

        break;
    case TransactionState::ResponseHeaders:

        break;
    default:
        notify_failure();
        break;
    }
}

void ProxyTransaction::impl::body_chunk_read(const std::string &buffer, size_t len)
{
    switch (state_)
    {
    case TransactionState::RequestHeaders:

        break;
    case TransactionState::ResponseHeaders:

        break;
    default:
        notify_failure();
        break;
    }
}

void ProxyTransaction::impl::notify_failure()
{
    parent_->notify_listeners([this](auto &listener)
    {
        listener->transaction_failed(*parent_);
    });
}

/////////////////////////////////

ProxyTransaction::ProxyTransaction(int id, std::shared_ptr<ConnectionPool> connectionPool, std::shared_ptr<Conn> clientConnection)
    : Transaction()
    , impl_(std::make_unique<impl>(id, connectionPool, clientConnection, this))
{
    impl_->begin();
}

int ProxyTransaction::id() const
{
    return impl_->id();
}

TransactionState ProxyTransaction::state() const
{
    return impl_->state();
}

std::error_code ProxyTransaction::error() const
{
    return impl_->error();
}

time_point ProxyTransaction::parse_http_date(const std::string &text)
{
    // per RFC 7231, we MUST accept dates in the following formats:
    // 1. IMF-fixdate (e.g. Sun, 06 Nov 1994 08:49:37 GMT)
    // 2. RFC 859     (e.g. Sunday, 06-Nov-94 08:49:37 GMT)
    // 3. asctime()   (e.g. Sun Nov  6 08:49:37 1994)

    // see std::get_time for format-string documentation
    const std::string IMF_FIXDATE = "%a, %d %b %Y %H:%M:%S GMT";
    const std::string RFC_850     = "%a, %d-%b-%y %H:%M:%S GMT";
    const std::string ASCTIME     = "%a %b %d %H:%M:%S %Y";

    for (auto &format : { IMF_FIXDATE, RFC_850, ASCTIME })
    {
        time_point tp = {};

        std::istringstream input(text);
        input.imbue(std::locale("C"));

        input >> date::parse(format, tp);

        if (input)
        {
            return tp;
        }
    }

    throw std::invalid_argument("Invalid date-time value");
}

