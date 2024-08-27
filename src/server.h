#pragma once
#include "session.h"

class server
{
public:
    server(net::io_context &ioc, tcp::endpoint endpoint);

private:
    void do_accept();
    void on_accept(std::shared_ptr<session> session,beast::error_code ec);
    tcp::acceptor acceptor_;
    net::io_context &ioc_;
};