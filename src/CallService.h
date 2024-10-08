/*************************************************************************
  > File Name: CallService.h
  > Author: zhongqf
  > Mail: zhongqf.cn@gmail.com
  > Created Time: 2024-08-28
 ************************************************************************/
#pragma once
#include "esl.h"
#include <string>
//#include <format>
#include "curl.h"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <list>
#include "Agent.h"
#include <memory>
#include "WebsocketSession.h"
#include "WebsocketServer.h"
#include <queue>
#include "CallEvent.h"
class CallService
{
private:
    static bool m_bRunning;
    std::list<std::shared_ptr<Agent>> m_listAgent;
    esl_handle_t m_handle;
    std::queue<std::shared_ptr<CallEvent>> m_queueEvent;
public:
    CallService();
    ~CallService();
    int loadconfig();
    bool startUp();
    void doService();
    void stopService();
    void hello() { std::cout << "hello from CallService" << std::endl; }
    std::list<std::shared_ptr<Agent>> &getAgentList() { return m_listAgent; }
    std::shared_ptr<Agent> get_available_agent();
    void reset_agents(std::shared_ptr<Agent> pAgent_);
    int makecall(std::string dn_,std::string dst_);
    int answercall(std::string dn_);
    int hangupcall(std::string dn_);
    void setHandle(esl_handle_t& handle_){m_handle = handle_;}
    esl_handle_t* getHandle(){return &m_handle;}
    void printAgent(std::shared_ptr<Agent> pAgent_);
public:
    static int HttpPost(const char *url, std::string data, std::string &response, int timeout = 0);
    static int HttpGet(const char *url, std::string param, std::string &resp);
    static int writedata(void *buffer, int size, int nmemb, void *userPtr);
private:
    static void callbackfun(esl_socket_t server_sock, esl_socket_t client_sock, struct sockaddr_in *addr, void *user_data);
    static void acd_callback(esl_socket_t server_sock, esl_socket_t client_sock, struct sockaddr_in *addr, void *user_data);
    static void *eventThreadFun(esl_thread_t *e, void *obj);
};
