#include "ConnectionPool.h"

#include <unordered_map>

using namespace ama;

namespace std
{

template <>
struct hash<std::pair<std::string, int>>
{
    std::size_t operator()(const std::pair<std::string, int> &pair)
    {
        std::size_t const h1 (std::hash<std::string>{}(pair.first));
        std::size_t const h2 (std::hash<int>{}(pair.second));
        return h1 ^ (h2 << 1);
    }
};

} // namespace std

Conn::Conn(asio::ip::tcp::socket &&socket)
    : socket_(std::move(socket))
    , expires_at_(time_point::max())
    , should_close_(false)
//    , pool_(nullptr)
{}

Conn::Conn(asio::io_service &service)
    : socket_(service)
    , expires_at_(time_point::max())
    , should_close_(false)
//    , pool_(nullptr)
{}

Conn::~Conn()
{
    socket_.close();
}

class ConnectionPool::impl : public std::enable_shared_from_this<ConnectionPool::impl>
{
public:
    impl(asio::io_service &, ConnectionPool *);

    std::shared_ptr<Conn> make_connection(asio::ip::tcp::socket &&socket);

    std::shared_ptr<Conn> find_open_connection(const std::string &host, int port);

    void try_open(const std::string &host, const std::string &port, std::function<void (std::shared_ptr<Conn>, std::error_code)> &&callback);

private:
    asio::ip::tcp::resolver resolver_;
    ConnectionPool *pool_;
}; // class ConnectionPool::impl

ConnectionPool::impl::impl(asio::io_service &service, ConnectionPool *pool)
    : resolver_(service)
    , pool_(pool)
{

}

std::shared_ptr<Conn> ConnectionPool::impl::make_connection(asio::ip::tcp::socket &&socket)
{
    auto connection = std::make_shared<Conn>(std::move(socket));
    pool_->notify_listeners([connection](auto &listener)
    {
        listener->on_client_connected(connection);
    });
    return connection;
}

std::shared_ptr<Conn> ConnectionPool::impl::find_open_connection(const std::string &host, int port)
{
    UNUSED(host);
    UNUSED(port);
    return std::shared_ptr<Conn>(nullptr);
}

void ConnectionPool::impl::try_open(const std::string &host, const std::string &port, std::function<void (std::shared_ptr<Conn>, std::error_code)> &&callback)
{
    auto self = shared_from_this();
    auto conn = std::make_shared<Conn>(resolver_.get_io_service());

    asio::ip::tcp::resolver::query query(host, port);

    resolver_.async_resolve(query, [this, self, conn, callback]
                            (asio::error_code ec, asio::ip::tcp::resolver::iterator result)
    {
        if (ec)
        {
            callback(nullptr, ec);
            return;
        }

        asio::async_connect(conn->socket_, result,
                            [this, self, conn, callback]
                            (asio::error_code ec, asio::ip::tcp::resolver::iterator /* i */)
        {
            if (ec)
            {
                callback(nullptr, ec);
                return;
            }

            callback(conn, ec);
        });
    });
}

ConnectionPool::ConnectionPool(asio::io_service &service)
    : impl_(std::make_shared<ConnectionPool::impl>(service, this))
{}

ConnectionPool::~ConnectionPool()
{

}

std::shared_ptr<Conn> ConnectionPool::make_connection(asio::ip::tcp::socket &&socket)
{
    return impl_->make_connection(std::move(socket));
}

std::shared_ptr<Conn> ConnectionPool::find_open_connection(const std::string &host, int port)
{
    return impl_->find_open_connection(host, port);
}

void ConnectionPool::try_open(const std::string &host, const std::string &port, std::function<void (std::shared_ptr<Conn>, std::error_code)> &&callback)
{
    impl_->try_open(host, port, std::move(callback));
}
