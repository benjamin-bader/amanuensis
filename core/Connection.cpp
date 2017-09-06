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

/*! \file Implements transparent HTTP proxy session per [RFC 7230](https://tools.ietf.org/html/rfc7230).
 *
 *
 */

#include "Connection.h"

#include <array>
#include <functional>
#include <mutex>
#include <queue>
#include <vector>

#include <QDebug>

#include <asio.hpp>

#include "ConnectionManager.h"
#include "Headers.h"
#include "HttpMessage.h"
#include "HttpMessageParser.h"
#include "Listenable.h"

using namespace ama;

namespace {
    QDebug operator<<(QDebug d, const std::string &str)
    {
        d << QString(str.c_str());
        return d;
    }

    void dump_request(const HttpMessage &req)
    {
        qDebug() << req.method() << req.uri() << " " << req.major_version() << "/" << req.minor_version();


        for (auto& header : req.headers())
        {
            qDebug() << header.first << ": " << header.second;
        }
    }

    struct Payload {
        BufferPtr buffer;
        size_t size;
    };

    std::list<std::string> hop_by_hop_headers = std::list<std::string> {
            "Connection",
            "Keep-Alive",
            "Proxy-Authenticate",
            "Proxy-Authorization",
            "TE",
            "Trailers",
            "Transfer-Encoding",
            "Upgrade"
    };
}

/**
 * @brief Holds implementations details of a Connection.
 *
 * The Connection state lifecycle is a little complicated,
 * because it attempts to perform certain proxy functions in
 * parallel with reading client requests:
 *
 * - As soon as a complete request line is received, we initiate
 *   a DNS query and a subsequent remote connection, buffering
 *   request data until the remote connection is established.
 *
 * - As soon as the remote connection is established, we begin
 *   sending buffered client data.  At the same time, we begin
 *   relaying data received from the remote back to the client.
 *
 * Thus, there are three independent "threads":
 * 1. A "thread" reading data from the client, parsing it, and
 *    subsequently feeding it into an output buffer.
 *
 * 2. A "thread" pulling from the output buffer and writing the
 *    data to the remote.
 *
 * 3. A "thread" reading from the remote, parsing it, and relaying
 *    the data directly to the client.
 *
 * There is no division of labor in the return direction, because
 * we don't need to perform work analogous to DNS resolution - the
 * connection from us to the client is already open.  Therefore we
 * can condense reading, parsing, and relaying into one fiber.
 *
 * @remarks
 * Because we expect that the client connection originates on the same
 * machine on which we are running, we don't bother to begin emitting
 * listener events (or forwarding data to the remote) until all headers
 * have been read.
 */
class Connection::impl : public Listenable<ConnectionListener>
{
public:
    impl(asio::ip::tcp::socket socket, std::shared_ptr<ConnectionManager> connectionManager) :
        id_(-1),
        socket_(std::move(socket)),
        remoteSocket_(socket_.get_io_service()),
        connectionManager_(connectionManager),
        outboxMutex_(),
        outbox_(),
        isSendingClientRequest_(false),
        serverToClientOutboxMutex_(),
        serverToClientOutbox_(),
        isSendingServerResponse_(false),
        requestParser_(),
        request_(),
        response_(),
        txState_(TransactionState::Start)
    {}

    int id_;

    asio::ip::tcp::socket socket_;
    asio::ip::tcp::socket remoteSocket_;

    std::shared_ptr<ConnectionManager> connectionManager_;

    std::mutex outboxMutex_;
    std::queue<Payload> outbox_;
    bool isSendingClientRequest_;

    std::mutex serverToClientOutboxMutex_;
    std::queue<Payload> serverToClientOutbox_;
    bool isSendingServerResponse_;

    HttpMessageParser requestParser_;
    HttpMessage request_;
    HttpMessage response_;

    TransactionState txState_;
};


Connection::Connection(asio::ip::tcp::socket socket, std::shared_ptr<ConnectionManager> connectionManager) :
    impl_(std::make_unique<Connection::impl>(std::move(socket), connectionManager))
{

}

Connection::~Connection()
{
}

int Connection::id() const
{
    return impl_->id_;
}

void Connection::set_id(int id)
{
    impl_->id_ = id;
}

// Impl

void Connection::start()
{
    do_read_client_request();
}

void Connection::stop()
{
    notify_connection_closing();

    asio::error_code ec;
    impl_->socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    impl_->remoteSocket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
}

void Connection::do_read_client_request()
{
    auto buffer = impl_->connectionManager_->takeBuffer();
    auto self(shared_from_this());
    impl_->socket_.async_read_some(asio::buffer(*buffer), [this, self, buffer](asio::error_code ec, size_t bytes_read) {
        if (ec == asio::error::eof)
        {
            qWarning() << "Unexpected end of request, abandoning intercept";
            do_write_client_request();
            return;
        }

        if (ec)
        {
            qWarning() << "Error reading from client, abandoning session: " << ec.message();
            impl_->connectionManager_->stop(self);
            return;
        }

        auto begin = buffer->begin();
        auto end = begin + bytes_read;
        auto parserState = impl_->requestParser_.parse(impl_->request_, begin, end);

        if (bytes_read > 0)
        {
            std::lock_guard<std::mutex> lock(impl_->outboxMutex_);
            impl_->outbox_.push(Payload { buffer, bytes_read });
        }

        if (parserState == HttpMessageParser::State::Valid)
        {
            dump_request(impl_->request_);

            if (impl_->request_.method() == "CONNECT")
            {
                do_tls_connect();
            }
            else
            {
                lookup_remote_host();
                notify_client_request_received();
            }
        }
        else if (parserState == HttpMessageParser::State::Incomplete)
        {
            do_read_client_request();
        }
        else if (parserState == HttpMessageParser::State::Invalid)
        {
            // d'oh, handle the error somehow
        }
    });
}

void Connection::lookup_remote_host()
{
    Headers::const_iterator iter = impl_->request_.headers().find_by_name("Host");
    if (iter == impl_->request_.headers().end())
    {
        qWarning() << "Malformed request - no 'Host' header found!";
        impl_->connectionManager_->stop(shared_from_this());
        return;
    }

    std::string host = iter->second;
    std::string port = "80";

    size_t separator = host.find(':');
    if (separator != std::string::npos)
    {
        try
        {
            port = host.substr(separator + 1);
            host = host.substr(0, separator);
        }
        catch (std::out_of_range)
        {
            qWarning() << "Malformed request - assuming port 80";
        }
        catch (std::invalid_argument)
        {
            qWarning() << "Malformed request - assuming port 80";
        }
    }

    asio::ip::tcp::resolver::query query(host, port);

    auto self = shared_from_this();
    impl_->connectionManager_->resolver().async_resolve(query, [this, self](asio::error_code ec, asio::ip::tcp::resolver::iterator result) {
        if (ec)
        {
            // Something happened - couldn't connect to the remote endpoint?
            // TODO(ben): Propagate the failure!

            qWarning() << "Failed to connect to the remote endpoint: " << ec.message();
            impl_->connectionManager_->stop(self);
            return;
        }

        qInfo() << "DNS lookup complete, connecting to remote host";
        asio::async_connect(impl_->remoteSocket_, result, [this, self](asio::error_code ec, asio::ip::tcp::resolver::iterator i) {
            if (ec)
            {
                qWarning() << "Failed to connect to remote host: " << ec.message();
                impl_->connectionManager_->stop(self);
                return;
            }

            qInfo() << "Connected to remote host " << i->host_name() << " at " << i->endpoint().address().to_string();

            do_write_client_request();
        });
    });
}

void Connection::do_write_client_request()
{
    Payload payload;

    {
        std::lock_guard<std::mutex> lock(impl_->outboxMutex_);
        if (impl_->outbox_.empty())
        {
            return;
        }

        payload = impl_->outbox_.front();
    }

    auto self = shared_from_this();
    asio::async_write(impl_->remoteSocket_,
                      asio::buffer(payload.buffer->data(), payload.size),
                      [this, self, payload](asio::error_code ec, size_t bytesWritten) {

        Q_UNUSED(bytesWritten);

        if (ec)
        {
            // boo
            qWarning() << "Error sending client request to server: " << ec.message();
            impl_->connectionManager_->stop(self);
            return;
        }

        bool finishedWriting;
        {
            std::lock_guard<std::mutex> lock(impl_->outboxMutex_);
            impl_->outbox_.pop();
            finishedWriting = impl_->outbox_.empty();
        }

        if (finishedWriting)
        {
            impl_->requestParser_.resetForResponse();
            do_read_server_response();
        }
        else
        {
            do_write_client_request();
        }
    });
}

void Connection::do_read_server_response()
{
    auto self = shared_from_this();
    auto buffer = impl_->connectionManager_->takeBuffer();
    impl_->remoteSocket_.async_read_some(asio::buffer(*buffer),
                                         [this, self, buffer](asio::error_code ec, size_t bytesRead) {
        qInfo() << "Received server chunk; ec=" << ec.message() << "bytesRead=" << bytesRead;
        if (ec == asio::error::eof)
        {
            // done reading, nothing more to do here
            qInfo() << "End of server response";
            do_write_server_response();
            return;
        }
        else if (ec)
        {
            qWarning() << "Error reading from server!  " << ec.message();
            impl_->connectionManager_->stop(self);
            return;
        }

        Payload payload{buffer, bytesRead};
        {
            std::lock_guard<std::mutex> lock(impl_->serverToClientOutboxMutex_);
            impl_->serverToClientOutbox_.push(payload);
        }

        auto begin = buffer->begin();
        auto end = begin + bytesRead;
        auto state = impl_->requestParser_.parse(impl_->response_, begin, end);

        if (state == HttpMessageParser::State::Incomplete)
        {
            do_read_server_response();
        }
        else if (state == HttpMessageParser::State::Valid)
        {
            do_write_server_response();

            notify_server_response_received();
        }
        else
        {
            qWarning() << "Couldn't understand server response, just pass the data through";
            do_read_server_response();
        }
    });
}

void Connection::do_write_server_response()
{
    Payload payload;

    {
        std::lock_guard<std::mutex> lock(impl_->serverToClientOutboxMutex_);
        if (impl_->isSendingServerResponse_)
        {
            qDebug() << "Send in progress; not starting another.";
        }

        if (impl_->serverToClientOutbox_.empty())
        {
            // What are we even doing here
            qFatal("Logic error; why do we have multiple threads waiting on an empty queue?");
        }

        payload = impl_->serverToClientOutbox_.front();
        impl_->isSendingServerResponse_ = true;
    }

    auto self = shared_from_this();
    asio::async_write(impl_->socket_,
                      asio::buffer(payload.buffer->data(), payload.size),
                      [this, self, payload](asio::error_code ec, size_t bytesWritten) {
        Q_UNUSED(bytesWritten);

        bool hasMore = false;
        {
            std::lock_guard<std::mutex> lock(impl_->serverToClientOutboxMutex_);
            impl_->serverToClientOutbox_.pop();
            hasMore = impl_->serverToClientOutbox_.size() > 0;

            impl_->isSendingServerResponse_ = false;
        }

        if (ec)
        {
            qWarning() << "nope: " << ec.message();
            impl_->connectionManager_->stop(self);
            return;
        }

        if (hasMore)
        {
            do_write_server_response();
        }
        else
        {
            // We're done!
            impl_->connectionManager_->stop(self);
        }
    });
}

void Connection::do_tls_connect()
{
    HttpMessage &request = impl_->request_;

    std::string host = request.uri();
    std::string port = "443";

    size_t separator = host.find(':');
    if (separator != std::string::npos)
    {
        try
        {
            port = host.substr(separator + 1);
            host = host.substr(0, separator);
        }
        catch (std::out_of_range)
        {
            qWarning() << "Malformed request - assuming port 443";
        }
        catch (std::invalid_argument)
        {
            qWarning() << "Malformed request - assuming port 443";
        }
    }

    asio::ip::tcp::resolver::query query(host, port);

    auto self = shared_from_this();
    impl_->connectionManager_->resolver().async_resolve(query,
                                                        [this, self, request](asio::error_code ec, asio::ip::tcp::resolver::iterator result) {
        if (ec)
        {
            qWarning() << "Error resolving remote host for CONNECT (" << request.uri() << "): " << ec.message();
            send_connect_response(false);
            return;
        }

        asio::async_connect(impl_->remoteSocket_, result, [this, self](asio::error_code ec, asio::ip::tcp::resolver::iterator i) {
            Q_UNUSED(i);

            if (ec)
            {
                qWarning() << "Failed to connect to remote host: " << ec.message();
                send_connect_response(false);
                return;
            }
            else
            {
                send_connect_response(true);
            }
        });
    });
}

void Connection::send_connect_response(bool success)
{
    std::string responseText;
    if (success) {
        responseText = "HTTP/1.1 200 OK\r\n"
                       "Proxy-Agent: amanuensis 0.1.0\r\n"
                       "\r\n";
    } else {
        responseText = "HTTP/1.1 400 Bad Request\r\n"
                       "Proxy-Agent: amanuensis 0.1.0\r\n"
                       "\r\n";
    }

    BufferPtr buffer = impl_->connectionManager_->takeBuffer();
    std::copy(responseText.begin(), responseText.end(), buffer->begin());

    auto self = shared_from_this();
    asio::async_write(impl_->socket_,
                      asio::buffer(*buffer, responseText.size()),
                      [this, self, success, buffer](asio::error_code ec, size_t bytes_written) {
        Q_UNUSED(bytes_written);

        if (ec)
        {
            qWarning() << "Error writing CONNECT response: " << ec.message();
            impl_->connectionManager_->stop(self);
            return;
        }

        if (success)
        {
            do_tls_client_to_server_forwarding();
            do_tls_server_to_client_forwarding();
        }
        else
        {
            // Failed to connect, so bail
            impl_->connectionManager_->stop(self);
        }
    });
}

void Connection::do_tls_client_to_server_forwarding()
{
    auto buffer = impl_->connectionManager_->takeBuffer();
    auto self = shared_from_this();
    impl_->socket_.async_read_some(asio::buffer(*buffer), [this, self, buffer](asio::error_code ec, size_t bytes_read) {
        if (bytes_read == 0 || ec)
        {
            impl_->connectionManager_->stop(self);
            return;
        }

        asio::async_write(impl_->remoteSocket_,
                          asio::buffer(buffer->data(), bytes_read),
                          [this, self, buffer](asio::error_code ec, size_t bytes_written) {
            if (ec == asio::error::eof)
            {
                impl_->connectionManager_->stop(self);
                return;
            }

            if (ec || bytes_written == 0)
            {
                impl_->connectionManager_->stop(self);
                return;
            }

            do_tls_client_to_server_forwarding();
        });
    });
}

void Connection::do_tls_server_to_client_forwarding()
{
    auto buffer = impl_->connectionManager_->takeBuffer();
    auto self = shared_from_this();
    impl_->remoteSocket_.async_read_some(asio::buffer(*buffer), [this, self, buffer](asio::error_code ec, size_t bytes_read) {
        if (bytes_read == 0 || ec)
        {
            impl_->connectionManager_->stop(self);
            return;
        }

        asio::async_write(impl_->socket_,
                          asio::buffer(buffer->data(), bytes_read),
                          [this, self, buffer](asio::error_code ec, size_t bytes_written) {
            if (ec || bytes_written == 0)
            {
                impl_->connectionManager_->stop(self);
                return;
            }

            do_tls_server_to_client_forwarding();
        });
    });
}

void Connection::notify_client_request_received()
{
    notify_listeners([this](auto &listener) {
        listener->client_request_received(shared_from_this(), impl_->request_);
    });
}

void Connection::notify_server_response_received()
{
    notify_listeners([this](auto &listener) {
        listener->server_response_received(shared_from_this(), impl_->response_);
    });
}

void Connection::notify_error(const std::error_code &error)
{
    notify_listeners([this, &error](auto &listener) {
        listener->on_error(shared_from_this(), error);
    });
}

void Connection::notify_connection_closing()
{
    notify_listeners([this](auto &listener) {
        listener->connection_closing(shared_from_this());
    });
}
