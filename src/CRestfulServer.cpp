/*************************************************************************
  > File Name: CRestfulServer.cpp
  > Author: zhongqf
  > Mail: zhongqf.cn@gmail.com
  > Created Time: 2024-08-28
 ************************************************************************/

#include "CRestfulServer.h"
#include <ranges>

CRestfulServer::CRestfulServer() {}
CRestfulServer::~CRestfulServer() {}
void CRestfulServer::runner(unsigned short port, void *argv)
{
    asio::io_context ioc;
    ip::tcp::acceptor acceptor(ioc, {ip::tcp::v4(), port});
    while (true)
    {
        ip::tcp::socket socket(ioc);
        acceptor.accept(socket);
        std::thread{std::bind(do_session, std::move(socket), argv, this)}.detach();
    }
}
bool CRestfulServer::parseCalldata(http::request<http::string_body>&req,std::string& Dn_,std::string& dst_,http::response<http::string_body>&res)
{
    rapidjson::Document doc;
    if (doc.Parse(req.body().c_str()).HasParseError())
    {
        res = boost::beast::http::response<boost::beast::http::string_body>(http::status::not_acceptable, req.version());
        res.body() = R"({"code":-1,"msg": "Data not acceptable."})";
        return false;
    }
    else
    {
        if (doc.HasMember("dn") && doc["dn"].IsString())
        {
            Dn_ = doc["dn"].GetString();
        }
        if (doc.HasMember("callee") && doc["callee"].IsString())
        {
            dst_ = doc["callee"].GetString();
        }
    }
    if (Dn_.empty() && dst_.empty())
    {
        res = boost::beast::http::response<boost::beast::http::string_body>(http::status::not_acceptable, req.version());
        res.body() = R"({"code":-1,"msg": "Data not acceptable."})";
        return false;
    }
    return true;
}
bool CRestfulServer::parseAgent(http::request<http::string_body> &req, std::string &Dn_, std::string &Agent_, http::response<http::string_body> &res)
{
    rapidjson::Document doc;
    if (doc.Parse(req.body().c_str()).HasParseError())
    {
        res = boost::beast::http::response<boost::beast::http::string_body>(http::status::not_acceptable, req.version());
        res.body() = R"({"code":-1,"msg": "Data not acceptable."})";
        return false;
    }
    else
    {
        if (doc.HasMember("dn") && doc["dn"].IsString())
        {
            Dn_ = doc["dn"].GetString();
        }
        if (doc.HasMember("agent") && doc["agent"].IsString())
        {
            Agent_ = doc["agent"].GetString();
        }
        if (Agent_.empty())
        {
            Agent_ = "9"+Dn_;
        }
    }
    if (Dn_.empty() && Agent_.empty())
    {
        res = boost::beast::http::response<boost::beast::http::string_body>(http::status::not_acceptable, req.version());
        res.body() = R"({"code":-1,"msg": "Data not acceptable."})";
        return false;
    }
    return true;
}

void CRestfulServer::do_session(ip::tcp::socket &socket, void *argv, void *self)
{
    try
    {
        CallService *pCallSrv = (CallService *)argv;
        CRestfulServer *pThis = (CRestfulServer *)self;
        pCallSrv->hello();
        boost::beast::flat_buffer buffer;
        http::request<http::string_body> req;
        boost::beast::error_code ec;
        auto bytes_transferred = http::read(socket, buffer, req, ec);
        http::response<http::string_body> res;
        res.set(boost::beast::http::field::content_type, "application/json");
        if (!ec)
        {
            if (req.method() == boost::beast::http::verb::get)
            {
                if (req.target() == "/users")
                {
                    res = boost::beast::http::response<boost::beast::http::string_body>(http::status::ok, req.version());
                    res.body() = R"({"users": ["Alice", "Bob"]})";
                }
                else
                {
                    res = boost::beast::http::response<boost::beast::http::string_body>(http::status::bad_request, req.version());
                    res.body() = R"({"code":-1,"msg": "Method Not Found."})";
                }
            }
            else if (req.method() == boost::beast::http::verb::post)
            {
                if (req.target() == "/agent/login")
                {
                    std::string dn, agent;
                    if (pThis->parseAgent(req, dn, agent, res))
                    {
                        std::shared_ptr<Agent> pAgent = std::make_shared<Agent>();
                        pAgent->setDn(dn);
                        pAgent->setAgent(agent);
                        pAgent->setPolling(false);
                        pAgent->setAgentStatus(AgentStatus_t::Ready);

                        auto it = std::find_if(pCallSrv->getAgentList().begin(), pCallSrv->getAgentList().end(), [&pAgent](const std::shared_ptr<Agent> &p)
                                               { return p->getDn() == pAgent->getDn(); });

                        if (it != pCallSrv->getAgentList().end())
                        {
                            std::shared_ptr<Agent> p = *it;
                            p->setAgent(agent);
                            p->setPolling(false);
                            p->setAgentStatus(AgentStatus_t::Ready);
                        }
                        else
                        {
                            pCallSrv->getAgentList().push_back(pAgent);
                        }
                        std::ranges::for_each(pCallSrv->getAgentList(), [](std::shared_ptr<Agent> p)
                                              { std::cout << "dn:" << p->getDn() << " agent:" << p->getAgent() << std::endl; });
                        res = boost::beast::http::response<boost::beast::http::string_body>(http::status::ok, req.version());
                        res.body() = R"({"code": 0,"msg":"success."})";
                    }
                }
                else if (req.target() == "/agent/logout")
                {
                    std::string dn, agent;
                    if (pThis->parseAgent(req, dn, agent, res))
                    {
                        std::shared_ptr<Agent> pAgent = std::make_shared<Agent>();
                        pAgent->setDn(dn);

                        auto it = std::find_if(pCallSrv->getAgentList().begin(), pCallSrv->getAgentList().end(), [&pAgent](const std::shared_ptr<Agent> &p)
                                               { return p->getDn() == pAgent->getDn(); });
                        if (it != pCallSrv->getAgentList().end())
                        {
                            pCallSrv->getAgentList().erase(it);
                            res = boost::beast::http::response<boost::beast::http::string_body>(http::status::ok, req.version());
                            res.body() = R"({"code": 0,"msg":"success."})";
                        }
                        else
                        {
                            res = boost::beast::http::response<boost::beast::http::string_body>(http::status::bad_request, req.version());
                            res.body() = R"({"code":-1,"msg": "Dn Not Found."})";
                        }
                        std::ranges::for_each(pCallSrv->getAgentList(), [](std::shared_ptr<Agent> p)
                                              { std::cout << "dn:" << p->getDn() << " agent:" << p->getAgent() << std::endl; });
                    }
                }
                else if (req.target() == "/agent/ready")
                {
                    std::string dn, agent;
                    if (pThis->parseAgent(req, dn, agent, res))
                    {
                        std::shared_ptr<Agent> pAgent = std::make_shared<Agent>();
                        pAgent->setDn(dn);
                         auto it = std::find_if(pCallSrv->getAgentList().begin(), pCallSrv->getAgentList().end(), [&pAgent](const std::shared_ptr<Agent> &p)
                                               { return p->getDn() == pAgent->getDn(); });
                        if (it != pCallSrv->getAgentList().end())
                        {
                            std::shared_ptr<Agent> p = *it;                           
                            p->setAgentStatus(AgentStatus_t::Ready);
                            res = boost::beast::http::response<boost::beast::http::string_body>(http::status::ok, req.version());
                            res.body() = R"({"code": 0,"msg":"success."})";
                        }
                        else
                        {
                            res = boost::beast::http::response<boost::beast::http::string_body>(http::status::bad_request, req.version());
                            res.body() = R"({"code":-1,"msg": "Dn Not Found."})";
                        }
                    }
                }
                else if (req.target() == "/agent/notReady")
                {
                    std::string dn, agent;
                    if (pThis->parseAgent(req, dn, agent, res))
                    {
                        std::shared_ptr<Agent> pAgent = std::make_shared<Agent>();
                        pAgent->setDn(dn);
                         auto it = std::find_if(pCallSrv->getAgentList().begin(), pCallSrv->getAgentList().end(), [&pAgent](const std::shared_ptr<Agent> &p)
                                               { return p->getDn() == pAgent->getDn(); });
                        if (it != pCallSrv->getAgentList().end())
                        {
                            std::shared_ptr<Agent> p = *it;                           
                            p->setAgentStatus(AgentStatus_t::notReady);
                            res = boost::beast::http::response<boost::beast::http::string_body>(http::status::ok, req.version());
                            res.body() = R"({"code": 0,"msg":"success."})";
                        }
                        else
                        {
                            res = boost::beast::http::response<boost::beast::http::string_body>(http::status::bad_request, req.version());
                            res.body() = R"({"code":-1,"msg": "Dn Not Found."})";
                        }
                    }
                }
                else if (req.target() == "/agent/leave")
                {
                    std::string dn, agent;
                    if (pThis->parseAgent(req, dn, agent, res))
                    {
                        std::shared_ptr<Agent> pAgent = std::make_shared<Agent>();
                        pAgent->setDn(dn);
                        auto it = std::find_if(pCallSrv->getAgentList().begin(), pCallSrv->getAgentList().end(), [&pAgent](const std::shared_ptr<Agent> &p)
                                               { return p->getDn() == pAgent->getDn(); });
                        if (it != pCallSrv->getAgentList().end())
                        {
                            std::shared_ptr<Agent> p = *it;                           
                            p->setAgentStatus(AgentStatus_t::leave);
                            res = boost::beast::http::response<boost::beast::http::string_body>(http::status::ok, req.version());
                            res.body() = R"({"code": 0,"msg":"success."})";
                        }
                        else
                        {
                            res = boost::beast::http::response<boost::beast::http::string_body>(http::status::bad_request, req.version());
                            res.body() = R"({"code":-1,"msg": "Dn Not Found."})";
                        }
                    }
                }
		else if(req.target()=="/call/makecall")
		{
                    std::string dn, obj;
                    if (pThis->parseCalldata(req, dn, obj, res))
		    {
			    pCallSrv->makecall(dn,obj);
		    }

		}
		else if(req.target()=="/call/answer")
		{
                    std::string dn, agent;
                    if (pThis->parseAgent(req, dn, agent, res))
		    {
			    pCallSrv->answercall(dn);
		    }

		}
		else if(req.target()=="/call/hangup")
		{
                    std::string dn, agent;
                    if (pThis->parseAgent(req, dn, agent, res))
		    {
			    pCallSrv->hangupcall(dn);
		    }

		}
                else
                {
                    res = boost::beast::http::response<boost::beast::http::string_body>(http::status::bad_request, req.version());
                    res.set(boost::beast::http::field::content_type, "application/json");
                    res.body() = R"({"code":-1,"msg": "Method Not Found"})";
                }
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
