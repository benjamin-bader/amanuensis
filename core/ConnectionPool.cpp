#include "ConnectionPool.h"

#include <unordered_map>
#include <utility>

#include <asio.hpp>


Conn::Conn(asio::ip::tcp::socket &&socket) :
    socket_(std::move(socket)),
    expires_at_(std::chrono::system_clock::time_point::max())
{

}


class ConnectionPool::impl
{
public:
    impl(asio::io_service &);

private:
    asio::ip::tcp::resolver resolver_;

};

ConnectionPool::impl::impl(asio::io_service &service) : resolver_(service)
{

}

ConnectionPool::ConnectionPool(asio::io_service &service)
    : impl_(std::make_unique<ConnectionPool::impl>(service))
{}



