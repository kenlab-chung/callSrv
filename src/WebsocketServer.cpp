/*************************************************************************
  > File Name: server.cpp
  > Author: zhongqf
  > Mail: zhongqf.cn@gmail.com 
  > Created Time: 2024-08-28
 ************************************************************************/

#include "WebsocketServer.h"

WebsocketServer::WebsocketServer(net::io_context &ioc, tcp::endpoint endpoint):acceptor_(ioc),ioc_(ioc)
{
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(net::socket_base::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();
    do_accept();
}
void WebsocketServer::do_accept()
{
    tcp::socket socket(ioc_);
    auto s = std::make_shared<WebsocketSession>(std::move(socket));
    acceptor_.async_accept(s->ws_.next_layer(),beast::bind_front_handler(&WebsocketServer::on_accept,this,s));
}
void WebsocketServer::on_accept(std::shared_ptr<WebsocketSession> session,beast::error_code ec)
{
    if(!ec)
    {
        session->run();
    }
    do_accept();
}