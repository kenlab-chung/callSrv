#include "session.h"

session::session(tcp::socket socket):ws_(std::move(socket))
{
}

session::~session()
{
}

void session::do_accept()
{
    ws_.set_option(websocket::stream_base::decorator([](websocket::response_type& res){
        res.set(http::field::server,std::string(BOOST_BEAST_VERSION_STRING)+"websocket-server-async");
    }));
    ws_.async_accept(beast::bind_front_handler(&session::on_accept,shared_from_this()));
}
void session::on_accept(beast::error_code ec)
{
    if(ec)
    {
        return fail(ec,"accept");
    }
    do_read();
}
void session::do_read()
{
    ws_.async_read(buffer_,beast::bind_front_handler(&session::on_read,shared_from_this()));
}
void session::on_read(beast::error_code ec,std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    if(ec)
    {
        return fail(ec,"read");
    }

    std::cout<<"Received: "<<beast::buffers_to_string(buffer_.data())<<std::endl;
    buffer_.consume(buffer_.size());
    do_read();
}
void session::fail(beast::error_code ec, char const *what)
{
    std::cerr<<what<<":"<<ec.message()<<std::endl;
}

void session::run()
{
    do_accept();
}
