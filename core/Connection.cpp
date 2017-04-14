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

#include "Connection.h"

#include <QDebug>

#include <asio.hpp>

#include "ConnectionManager.h"
#include "Headers.h"
#include "HttpMessage.h"

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
}

Connection::Connection(asio::ip::tcp::socket socket, std::shared_ptr<ConnectionManager> connectionManager) :
    socket_(std::move(socket)),
    remoteSocket_(socket_.get_io_service()),
    connectionManager_(connectionManager),
    requestParser(),
    request()
{

}

void Connection::start()
{
    do_read_client_request();
}

void Connection::stop()
{
    asio::error_code ec;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    remoteSocket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
}

void Connection::do_read_client_request()
{
    auto buffer = connectionManager_->takeBuffer();
    auto self(shared_from_this());
    socket_.async_read_some(asio::buffer(*buffer), [this, self, buffer](asio::error_code ec, size_t bytes_read) {
        if (ec == asio::error::eof)
        {
            qWarning() << "Unexpected end of request, abandoning intercept";
            do_write_client_request();
            return;
        }

        if (ec)
        {
            qWarning() << "Error reading from client, abandoning session: " << ec.message();
            connectionManager_->stop(self);
            return;
        }

        auto begin = buffer->begin();
        auto end = begin + bytes_read;
        auto parserState = requestParser.parse(request, begin, end);

        if (bytes_read > 0)
        {
            std::lock_guard<std::mutex> lock(outboxMutex_);
            outbox_.push(Payload { buffer, bytes_read });
        }

        if (parserState == HttpMessageParser::State::Valid)
        {
            dump_request(request);

            lookup_remote_host();

            notify_client_request_received();
        }
        if (parserState == HttpMessageParser::State::Incomplete)
        {
            do_read_client_request();
        }
        else if (parserState == HttpMessageParser::State::Invalid)
        {
            // d'oh, handle the error somehow
        }
        else
        {

        }
    });
}

void Connection::lookup_remote_host()
{
    Headers::const_iterator iter = request.headers().find_by_name("Host");
    if (iter == request.headers().end())
    {
        qWarning() << "Malformed request - no 'Host' header found!";
        connectionManager_->stop(shared_from_this());
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
    connectionManager_->resolver().async_resolve(query, [this, self](asio::error_code ec, asio::ip::tcp::resolver::iterator result) {
        if (ec)
        {
            // Something happened - couldn't connect to the remote endpoint?
            // TODO(ben): Propagate the failure!

            qWarning() << "Failed to connect to the remote endpoint: " << ec.message();
            connectionManager_->stop(self);
            return;
        }

        qInfo() << "DNS lookup complete, connecting to remote host";
        connect_to_remote_server(result);
    });
}

void Connection::connect_to_remote_server(asio::ip::tcp::resolver::iterator result)
{
    auto self = shared_from_this();
    asio::async_connect(remoteSocket_, result, [this, self](asio::error_code ec, asio::ip::tcp::resolver::iterator i) {
        if (ec)
        {
            qWarning() << "Failed to connect to remote host: " << ec.message();
            connectionManager_->stop(self);
            return;
        }

        qInfo() << "Connected to remote host " << i->host_name() << " at " << i->endpoint().address().to_string();

        do_write_client_request();
    });
}

void Connection::do_write_client_request()
{
    Payload payload;

    {
        std::lock_guard<std::mutex> lock(outboxMutex_);
        if (outbox_.empty())
        {
            return;
        }

        payload = outbox_.front();
    }

    auto self = shared_from_this();
    asio::async_write(remoteSocket_,
                      asio::buffer(payload.buffer->data(), payload.size),
                      [this, self, payload](asio::error_code ec, size_t bytesWritten) {

        Q_UNUSED(bytesWritten);

        if (ec)
        {
            // boo
            qWarning() << "Error sending client request to server: " << ec.message();
            connectionManager_->stop(self);
            return;
        }

        bool finishedWriting;
        {
            std::lock_guard<std::mutex> lock(outboxMutex_);
            outbox_.pop();
            finishedWriting = outbox_.empty();
        }

        if (finishedWriting)
        {
            requestParser.resetForResponse();
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
    auto buffer = connectionManager_->takeBuffer();
    remoteSocket_.async_read_some(asio::buffer(*buffer),
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
            connectionManager_->stop(self);
            return;
        }

        Payload payload{buffer, bytesRead};
        {
            std::lock_guard<std::mutex> lock(serverToClientOutboxMutex_);
            serverToClientOutbox_.push(payload);
        }

        auto begin = buffer->begin();
        auto end = begin + bytesRead;
        auto state = requestParser.parse(response, begin, end);

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
        std::lock_guard<std::mutex> lock(serverToClientOutboxMutex_);
        if (isSendingServerResponse)
        {
            qDebug() << "Send in progress; not starting another.";
        }

        if (serverToClientOutbox_.empty())
        {
            // What are we even doing here
            qFatal("Logic error; why do we have multiple threads waiting on an empty queue?");
        }

        payload = serverToClientOutbox_.front();
        this->isSendingServerResponse = true;
    }

    auto self = shared_from_this();
    asio::async_write(socket_,
                      asio::buffer(payload.buffer->data(), payload.size),
                      [this, self, payload](asio::error_code ec, size_t bytesWritten) {
        Q_UNUSED(bytesWritten);

        bool hasMore = false;
        {
            std::lock_guard<std::mutex> lock(serverToClientOutboxMutex_);
            serverToClientOutbox_.pop();
            hasMore = serverToClientOutbox_.size() > 0;

            isSendingServerResponse = false;
        }

        if (ec)
        {
            qWarning() << "nope: " << ec.message();
            connectionManager_->stop(self);
            return;
        }

        if (hasMore)
        {
            do_write_server_response();
        }
        else
        {
            // We're done!
            connectionManager_->stop(self);
        }
    });
}

void Connection::notify_client_request_received()
{
    notify_listeners([this](auto &listener) {
        listener->client_request_received(request);
    });
}

void Connection::notify_server_response_received()
{
    notify_listeners([this](auto &listener) {
        listener->server_response_received(response);
    });
}
