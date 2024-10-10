/*************************************************************************
  > File Name: session.cpp
  > Author: zhongqf
  > Mail: zhongqf.cn@gmail.com 
  > Created Time: 2024-08-28
 ************************************************************************/

#include "WebsocketSession.h"

WebsocketSession::WebsocketSession(tcp::socket socket) : ws_(std::move(socket))
{
}

WebsocketSession::~WebsocketSession()
{
}

void WebsocketSession::do_accept()
{
    ws_.set_option(websocket::stream_base::decorator([](websocket::response_type &res)
                                                     { res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + "websocket-server-async"); }));
    ws_.async_accept(beast::bind_front_handler(&WebsocketSession::on_accept, shared_from_this()));
}
void WebsocketSession::do_write(std::string message)
{
    ws_.async_write(net::buffer(message),beast::bind_front_handler(&WebsocketSession::on_write,shared_from_this()));
}
void WebsocketSession::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    if(ec){
        return fail(ec,"write");
    }
}
void WebsocketSession::on_accept(beast::error_code ec)
{
    if (ec)
    {
        return fail(ec, "accept");
    }
    do_read();
}
void WebsocketSession::do_read()
{
    std::cout<<"do_read()"<<std::endl;
    ws_.async_read(buffer_, beast::bind_front_handler(&WebsocketSession::on_read, shared_from_this()));
}
void WebsocketSession::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    std::cout<<"on_read()"<<std::endl;
    boost::ignore_unused(bytes_transferred);
    if (ec)
    {
        return fail(ec, "read");
    }

    std::cout << "Received: " << beast::buffers_to_string(buffer_.data()) << std::endl;
    do_write(beast::buffers_to_string(buffer_.data()));
    buffer_.consume(buffer_.size());
    do_read();
}
void WebsocketSession::fail(beast::error_code ec, char const *what)
{
    std::cerr << what << ":" << ec.message() << std::endl;
}

void WebsocketSession::run()
{
    do_accept();
}
