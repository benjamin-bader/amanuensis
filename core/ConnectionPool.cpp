#include "ConnectionPool.h"

#include <utility>

#include <asio.hpp>

class ConnectionPool::impl
{
public:
    impl(asio::ip::tcp::socket &&socket);

private:
    asio::ip::tcp::socket socket_;

    std::chrono::time_point expires_at_;
};

ConnectionPool::impl::impl(asio::ip::tcp::socket &&socket) :
    socket_(std::move(socket)),
    expires_at_(std::chrono::time_point())
{

}

ConnectionPool::ConnectionPool(asio::ip::tcp::socket &&socket) :
    impl_(std::make_unique<ConnectionPool::impl>(std::move(socket)))
{

}
