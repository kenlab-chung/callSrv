/*************************************************************************
  > File Name: CallService.h
  > Author: zhongqf
  > Mail: zhongqf.cn@gmail.com
  > Created Time: 2024-08-28
 ************************************************************************/
#pragma once
#include "esl.h"
#include <string>
#include <format>
#include "curl.h"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <list>
#include "Agent.h"
#include <memory>
#include "WebsocketSession.h"
#include "WebsocketServer.h"

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
    void hello() { std::cout << "hello from CallService" << std::endl; }
    std::list<std::shared_ptr<Agent>> &getAgentList() { return m_listAgent; }

private:
    static void callbackfun(esl_socket_t server_sock, esl_socket_t client_sock, struct sockaddr_in *addr, void *user_data);
    static void *eventThreadFun(esl_thread_t *e, void *obj);
    int HttpPost(const char *url, std::string data, std::string &response, int timeout = 0);
    int HttpGet(const char *url, std::string param, std::string &resp);
    static int writedata(void *buffer, int size, int nmemb, void *userPtr);
};
