/*************************************************************************
  > File Name: CallService.cpp
  > Author: zhongqf
  > Mail: zhongqf.cn@gmail.com
  > Created Time: 2024-08-28
 ************************************************************************/
#include "CallService.h"
#include <unistd.h>
#include "CRestfulServer.h"
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/executor_work_guard.hpp>

bool CallService::m_bRunning = false;
CallService::CallService(/* args */)
{
    m_bRunning = false;
}

CallService::~CallService()
{
}

int CallService::loadconfig()
{
    return 0;
}

bool CallService::startUp()
{
    // net::io_context ioc;
    // WebsocketServer s(ioc,tcp::endpoint(net::ip::make_address("0.0.0.0"),8081));
    // ioc.run();

    m_bRunning = true;

    std::thread{std::bind([this](int port)
                          {
        CRestfulServer server;
        server.runner(port,this); }, 8080)}
        .detach();

    esl_global_set_default_logger(7);
    std::thread{
        std::bind([this]()
                  { esl_listen_threaded("192.168.1.91", 8040, CallService::callbackfun, this, 100000); })}
        .detach();

    std::thread{
        std::bind([this]()
                  {
                      esl_handle_t handle = {0};
                      while (true)
                      {
                          if (esl_connect_timeout(&handle, "192.168.1.2", 8021, "", "ClueCon", 3000))
                          {
                              esl_log(ESL_LOG_ERROR, "Error Connecteling [%s]\n", handle.err);
                              continue;
                          }
                          break;
                      }

                      esl_thread_create_detached(CallService::eventThreadFun, &handle);
                      esl_events(&handle, ESL_EVENT_TYPE_PLAIN, "ALL");
                     // esl_events(&handle, ESL_EVENT_TYPE_PLAIN, "CHANNEL_HANGUP_COMPLETE");

                      char cmd_str[2048] = {0};
                      snprintf(cmd_str, sizeof(cmd_str), "api version\n\n");
                      esl_send_recv(&handle, cmd_str);
                      if (handle.last_sr_event && handle.last_sr_event->body) {
                         esl_log(ESL_LOG_INFO, "%s\n", handle.last_sr_event->body);
                     }

                     snprintf(cmd_str, sizeof(cmd_str), "api originate user/1003 &echo\n\n");
                    esl_send_recv(&handle, cmd_str);
                    if (handle.last_sr_event && handle.last_sr_event->body) {
                        esl_log(ESL_LOG_INFO, "%s\n", handle.last_sr_event->body);
                    }
                    
                      while (this->m_bRunning)
                      {
                          std::this_thread::sleep_for(std::chrono::microseconds(1000));
                      }

                      esl_disconnect(&handle); })}

        .detach();
    return true;
}
void *CallService::eventThreadFun(esl_thread_t *e, void *obj)
{
    esl_handle_t *handle = (esl_handle_t *)obj;
    while (handle->connected)
    {
        esl_status_t status;
        status = esl_recv_event_timed(handle, 10, 1, NULL);
        switch (status)
        {
        case ESL_BREAK:
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            break;
        case ESL_FAIL:
            esl_log(ESL_LOG_WARNING, "Disconnected.\n");
            break;
        case ESL_SUCCESS:
            esl_log(ESL_LOG_INFO, "coming event_body:%xs\n", handle->last_event);
            if (handle->last_event->body)
            {
                esl_log(ESL_LOG_INFO, "event_body:%s\n", handle->last_event->body);
            }
            break;
        }
    }
    return nullptr;
}

void CallService::doService()
{
    while (m_bRunning)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
}

void CallService::stopService()
{
    m_bRunning = false;
}

void CallService::callbackfun(esl_socket_t server_sock, esl_socket_t client_sock, sockaddr_in *addr, void *user_data)
{
    CallService *pThis = (CallService *)user_data;
    esl_handle_t handle = {{0}};
    int done = 0;
    esl_status_t status;
    time_t exp = 0;
    esl_attach_handle(&handle, client_sock, addr);
    esl_log(ESL_LOG_INFO, "Connected! %d\n", handle.sock);

    esl_filter(&handle, "unique-id", esl_event_get_header(handle.info_event, "caller-unique-id"));
    esl_events(&handle, ESL_EVENT_TYPE_PLAIN, "SESSION_HEARTBEAT CHANNEL_ANSWER CHANNEL_ORIGINATE CHANNEL_PROGRESS CHANNEL_HANGUP "
                                              "CHANNEL_BRIDGE CHANNEL_UNBRIDGE CHANNEL_OUTGOING CHANNEL_EXECUTE CHANNEL_EXECUTE_COMPLETE DTMF CUSTOM conference::maintenance");

    esl_send_recv(&handle, "linger");
    esl_execute(&handle, "answer", NULL, NULL);
    // esl_execute(&handle, "conference", "3000@default", NULL);
    // esl_execute(&handle, "transfer", "1001 XML default", NULL);
    esl_execute(&handle, "playback", "/usr/local/freeswitch/sound/welcome.wav", NULL);
    for (auto &agent_ : pThis->getAgentList())
    {
        if (agent_->getPolling() == false)
        {
            agent_->setPolling(true);
            esl_execute(&handle, "bridge", std::format("user/{}", agent_->getDn().c_str()).c_str(), NULL);
            break;
        }
    }

    auto it = std::find_if(pThis->getAgentList().begin(), pThis->getAgentList().end(), [](std::shared_ptr<Agent> &agent_)
                           { return agent_->getPolling() == false; });
    if (it == pThis->getAgentList().end())
    {
        std::for_each(pThis->getAgentList().begin(), pThis->getAgentList().end(), [](auto x)
                      { x->setPolling(false); });
    }
    while ((status = esl_recv_timed(&handle, 1000)) != ESL_FAIL)
    {
        if (done)
        {
            if (time(NULL) >= exp)
            {
                break;
            }
        }
        else if (status == ESL_SUCCESS)
        {
            const char *type = esl_event_get_header(handle.last_event, "content-type");
            if (type && !strcasecmp(type, "text/disconnect-notice"))
            {
                const char *dispo = esl_event_get_header(handle.last_event, "content-disposition");
                esl_log(ESL_LOG_INFO, "Got a disconnection notice dispostion: [%s]\n", dispo ? dispo : "");
                if (dispo && !strcmp(dispo, "linger"))
                {
                    done = 1;
                    esl_log(ESL_LOG_INFO, "Waiting 5 seconds for any remaining events.\n");
                    exp = time(NULL) + 5;
                }
            }
        }
    }
    esl_log(ESL_LOG_INFO, "Disconnectd!%d\n", handle.sock);

    esl_disconnect(&handle);
}

int CallService::HttpPost(const char *url, std::string data, std::string &response, int timeout)
{
    if (timeout <= 0)
        timeout = 20;
    CURLcode res = CURLE_FAILED_INIT;
    CURL *curl = NULL;
    std::string resp;
    curl = curl_easy_init();

    if (curl == NULL)
    {
        const char *szError = curl_easy_strerror(res);
        return (int)CURLE_FAILED_INIT;
    }
    curl_slist *http_headers = NULL;
    http_headers = curl_slist_append(http_headers, "Accept: application/json");
    http_headers = curl_slist_append(http_headers, "Content-Type: application/json");
    http_headers = curl_slist_append(http_headers, "charsets: utf-8");
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CallService::writedata);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (char *)&resp);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        const char *szError = curl_easy_strerror(res);
    }
    else
    {
        response = resp;
    }
    curl_slist_free_all(http_headers);
    curl_easy_cleanup(curl);
    return (int)res;
}

int CallService::HttpGet(const char *url, std::string param, std::string &resp)
{
    CURL *curl = NULL;
    CURLcode res = CURLE_FAILED_INIT;
    curl = curl_easy_init();
    if (curl == NULL)
    {
        const char *szError = curl_easy_strerror(res);
        return (int)CURLE_FAILED_INIT;
    }
    struct curl_slist *headerlist = NULL;
    headerlist = curl_slist_append(headerlist, "Content-Type:application/x-www-form-urlencoded;charset=UTF-8");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, param);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CallService::writedata);
    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (char *)&response);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        const char *szError = curl_easy_strerror(res);
    }
    else
    {
        resp = response;
    }
    curl_slist_free_all(headerlist);
    curl_easy_cleanup(curl);
    return (int)res;
}

int CallService::writedata(void *buffer, int size, int nmemb, void *userPtr)
{
    std::string *str = static_cast<std::string *>((std::string *)userPtr);
    str->append((char *)buffer, size * nmemb);
    return nmemb;
}
