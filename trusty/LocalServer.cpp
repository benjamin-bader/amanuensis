#include "TrustyServer.h"

#include <asio.hpp>

struct TrustyServer::impl :
        public IService,
        public std::enable_shared_from_this<TrustyServer::impl>
{
public:
    impl(const std::shared_ptr<SystemLogger> &logger, int fd);
    ~impl();

    void run();

private:
    void do_accept();

    std::shared_ptr<SystemLogger> logger_;

    asio::io_service io_service_;
    asio::signal_set signal_set_;
    asio::local::stream_protocol::acceptor acceptor_;
    asio::local::stream_protocol::socket socket_;
};

TrustyServer::impl::impl(const std::shared_ptr<SystemLogger> &logger, int fd) :
    logger_(logger),
    io_service_(),
    signal_set_(io_service_),
    acceptor_(io_service_),
    socket_(io_service_)
{
    acceptor_.assign(asio::local::stream_protocol(), fd);
    do_accept();
}

TrustyServer::impl::~impl()
{

}

void TrustyServer::impl::do_accept()
{
    auto self = shared_from_this()
}

void TrustyServer::impl::run()
{
    io_service_.run();
}

TrustyServer::TrustyServer(int socket_fd) :
    OSLoggable("com.bendb.amanuensis.Trusty", "TrustyServer"),
    impl_(std::make_shared<impl>(this->logger(), socket_fd))
{
}

TrustyServer::~TrustyServer()
{
}

void TrustyServer::run()
{
    impl_->run();
}

void TrustyServer::set_http_proxy_host(const std::string &host)
{

}

void TrustyServer::set_http_proxy_port(int port)
{

}
