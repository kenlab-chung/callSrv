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

class session : public std::enable_shared_from_this<session>
{
public:
    session(tcp::socket socket);
    ~session();
    void run();

private:
    void do_accept();
    void on_accept(beast::error_code ec);
    void do_read();
    void on_read(beast::error_code ec,std::size_t bytes_transferred);
    void fail(beast::error_code ec,char const* what);
public:
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
};
