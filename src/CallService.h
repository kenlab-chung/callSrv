#pragma once
#include "esl.h"
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
};

