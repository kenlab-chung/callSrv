#include "CRestfulServer.h"
CRestfulServer::CRestfulServer() {}
CRestfulServer::~CRestfulServer() {}
void CRestfulServer::runner(unsigned short port)
{
    asio::io_context ioc;
    ip::tcp::acceptor acceptor(ioc, {ip::tcp::v4(), port});
    while (true)
    {
        ip::tcp::socket socket(ioc);
        acceptor.accept(socket);
        std::thread{std::bind(do_session, std::move(socket))}.detach();
    }
}
void CRestfulServer::do_session(ip::tcp::socket &socket)
{
    try
    {
        boost::beast::flat_buffer buffer;
        http::request<http::string_body> req;
        boost::beast::error_code ec;
        auto bytes_transferred = http::read(socket, buffer, req, ec);
        http::response<http::string_body> res;
        if (!ec)
        {
            if (req.method() == boost::beast::http::verb::get)
            {
                if (req.target() == "/users")
                {
                    res = boost::beast::http::response<boost::beast::http::string_body>(http::status::ok, req.version());
                    res.set(http::field::content_type, "application/json");
                    res.body() = R"({"users": ["Alice", "Bob"]})";
                }
                else
                {
                    res = boost::beast::http::response<boost::beast::http::string_body>(http::status::bad_request, req.version());
                    res.set(boost::beast::http::field::content_type, "application/json");
                    res.body() = R"({"msg": ["data", "Not Found"]})";
                }
            }
            else if (req.method() == boost::beast::http::verb::post)
            {
            }
            else if (req.method() == boost::beast::http::verb::put)
            {
            }
            else if (req.method() == boost::beast::http::verb::delete_)
            {
            }
            else
            {
                res = boost::beast::http::response<boost::beast::http::string_body>(boost::beast::http::status::not_implemented, req.version());
            }

            res.prepare_payload();
            http::write(socket, res);
        }
    }
    catch (const std::exception &e)
    {
        std::cout << std::string(e.what()) << std::endl;
    }
}
