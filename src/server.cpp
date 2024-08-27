#include "server.h"

server::server(net::io_context &ioc, tcp::endpoint endpoint):acceptor_(ioc),ioc_(ioc)
{
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(net::socket_base::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();
    do_accept();
}
void server::do_accept()
{
    tcp::socket socket(ioc_);
    auto s = std::make_shared<session>(std::move(socket));
    acceptor_.async_accept(s->ws_.next_layer(),beast::bind_front_handler(&server::on_accept,this,s));
}
void server::on_accept(std::shared_ptr<session> session,beast::error_code ec)
{
    if(!ec)
    {
        session->run();
    }
    do_accept();
}