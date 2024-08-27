#pragma once
#include "esl.h"
#include <string>
#include "curl.h"
#include <iostream>
#include <list>
#include "Agent.h"
#include <memory>
#include "session.h"
#include "server.h"
class CallService
{
private:
    static bool m_bRunning;    
    std::list<std::shared_ptr<Agent>> m_listAgent;
public:
    CallService();
    ~CallService();
    int loadconfig();
    bool startUp();
    void doService();
    void stopService();
    void hello(){std::cout<<"hello from CallService"<<std::endl;}
    std::list<std::shared_ptr<Agent>>& getAgentList(){return m_listAgent;}
private:
    void static callbackfun(esl_socket_t server_sock,esl_socket_t client_sock,struct sockaddr_in *addr,void *user_data);
    int HttpPost(const char* url,std::string data,std::string& response,int timeout=0);
    int HttpGet(const char* url,std::string param,std::string& resp);
    int static  writedata(void* buffer,int size,int nmemb,void* userPtr);    
};

