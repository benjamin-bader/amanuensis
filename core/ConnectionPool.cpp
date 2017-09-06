#include "ConnectionPool.h"

#include <functional>
#include <unordered_map>
#include <utility>

#include <asio.hpp>

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

}

Conn::Conn(asio::ip::tcp::socket &&socket) :
    socket_(std::move(socket)),
    expires_at_(std::chrono::system_clock::time_point::max())
{

}

namespace
{

class ConnectionImpl : public Connection
{

};

} // namespace


class ConnectionPool::impl
{
public:
    typedef std::pair<std::string, int> endpoint;

    impl(asio::io_service &);

private:
    asio::ip::tcp::resolver resolver_;

    std::unordered_multimap<endpoint, Conn*> pool_;
};

ConnectionPool::impl::impl(asio::io_service &service) : resolver_(service)
{

}

ConnectionPool::ConnectionPool(asio::io_service &service)
    : impl_(std::make_unique<ConnectionPool::impl>(service))
{}



