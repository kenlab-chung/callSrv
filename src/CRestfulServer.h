#pragma once
#include <boost/beast/version.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <memory>


namespace asio = boost::asio;
namespace ip = asio::ip;
namespace http = boost::beast::http;


class CRestfulServer :public std::enable_shared_from_this<CRestfulServer>{
public:
    CRestfulServer();
    ~CRestfulServer();
    void runner(unsigned short port);
    static void do_session(ip::tcp::socket& socket);
};
