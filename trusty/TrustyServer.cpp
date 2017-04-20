#include "TrustyServer.h"

#include <asio.hpp>

struct TrustyServer::impl
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

}

void TrustyServer::impl::run()
{
    io_service_.run();
}

TrustyServer::TrustyServer(int socket_fd) :
    OSLoggable("com.bendb.amanuensis.Trusty", "TrustyServer"),
    impl_(std::make_unique<impl>(this->logger(), socket_fd))
{
}

TrustyServer::~TrustyServer()
{
}

void TrustyServer::run()
{
    impl_->run();
}
