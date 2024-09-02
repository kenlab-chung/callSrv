/*************************************************************************
  > File Name: server.h
  > Author: zhongqf
  > Mail: zhongqf.cn@gmail.com 
  > Created Time: 2024-08-28
 ************************************************************************/
#pragma once
#include "WebsocketSession.h"

class WebsocketServer
{
public:
    WebsocketServer(net::io_context &ioc, tcp::endpoint endpoint);

private:
    void do_accept();
    void on_accept(std::shared_ptr<WebsocketSession> session,beast::error_code ec);
    tcp::acceptor acceptor_;
    net::io_context &ioc_;
};