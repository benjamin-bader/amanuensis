#include "LocalServer.h"

#include <asio.hpp>

struct LocalServer::impl :
        public IService,
        public std::enable_shared_from_this<LocalServer::impl>
{
public:
    impl(const std::shared_ptr<SystemLogger> &logger, int fd);
    ~impl();

    void run();

    virtual void set_http_proxy_host(const std::string &host) override;
    virtual void set_http_proxy_port(int port) override;

private:
    void do_accept();

    std::shared_ptr<SystemLogger> logger_;

    asio::io_service io_service_;
    asio::signal_set signal_set_;
    asio::local::stream_protocol::acceptor acceptor_;
    asio::local::stream_protocol::socket socket_;
};

LocalServer::impl::impl(const std::shared_ptr<SystemLogger> &logger, int fd) :
    logger_(logger),
    io_service_(),
    signal_set_(io_service_),
    acceptor_(io_service_),
    socket_(io_service_)
{
    acceptor_.assign(asio::local::stream_protocol(), fd);
    do_accept();
}

LocalServer::impl::~impl()
{

}

void LocalServer::impl::do_accept()
{
    auto self = shared_from_this();
}

void LocalServer::impl::run()
{
    io_service_.run();
}

void LocalServer::impl::set_http_proxy_host(const std::string &host)
{
    (void)host;
}

void LocalServer::impl::set_http_proxy_port(int port)
{
    (void)port;
}

////////////

LocalServer::LocalServer(int socket_fd) :
    OSLoggable("com.bendb.amanuensis.Trusty", "TrustyServer"),
    impl_(std::make_shared<impl>(this->logger(), socket_fd))
{
}

LocalServer::~LocalServer()
{
}

void LocalServer::run()
{
    impl_->run();
}

void LocalServer::set_http_proxy_host(const std::string &host)
{
    impl_->set_http_proxy_host(host);
}

void LocalServer::set_http_proxy_port(int port)
{
    impl_->set_http_proxy_port(port);;
}
