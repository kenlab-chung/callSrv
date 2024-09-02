/*************************************************************************
  > File Name: CRestfulServer.h
  > Author: zhongqf
  > Mail: zhongqf.cn@gmail.com 
  > Created Time: 2024-08-28
 ************************************************************************/

#pragma once
#include "CallService.h"
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
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>

namespace asio = boost::asio;
namespace ip = asio::ip;
namespace http = boost::beast::http;

class CRestfulServer :public std::enable_shared_from_this<CRestfulServer>{
public:
    CRestfulServer();
    ~CRestfulServer();
    void runner(unsigned short port,void* argv);
    static void do_session(ip::tcp::socket& socket,void* argv,void *self);
    bool parseAgent(http::request<http::string_body>&req,std::string& Dn_,std::string& Agent_,http::response<http::string_body>&res);
};
