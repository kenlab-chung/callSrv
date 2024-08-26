#pragma once
#include "esl.h"
#include <string>
#include "curl.h"

class CallService
{
private:
    static bool m_bRunning;
public:
    CallService();
    ~CallService();
    int loadconfig();
    bool startUp();
    void doService();
    void stopService();
private:
    void static callbackfun(esl_socket_t server_sock,esl_socket_t client_sock,struct sockaddr_in *addr,void *user_data);
    int HttpPost(const char* url,std::string data,std::string& response,int timeout=0);
    int HttpGet(const char* url,std::string param,std::string& resp);
    int static  writedata(void* buffer,int size,int nmemb,void* userPtr);
};

