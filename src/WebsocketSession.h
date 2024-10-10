/*************************************************************************
  > File Name: session.h
  > Author: zhongqf
  > Mail: zhongqf.cn@gmail.com
  > Created Time: 2024-08-28
 ************************************************************************/

#pragma once
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class WebsocketSession : public std::enable_shared_from_this<WebsocketSession>
{
public:
    WebsocketSession(tcp::socket socket);
    ~WebsocketSession();
    void run();

public:
    void do_write(std::string message);

private:
    void do_accept();
    void on_accept(beast::error_code ec);
    void do_read();

    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void fail(beast::error_code ec, char const *what);

public:
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
};
