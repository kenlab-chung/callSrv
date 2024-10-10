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
#include <pqxx/pqxx>
#include <regex>

/******************************************************
 * postgresql dev lib
 * sudo apt-get install libpq-dev
 * git clone https://github.com/jtv/libpqxx.git
 * mkdir build && cd build
 * cmake .. -DCMAKE_INSTALL_PREFIX=/opt/libpqxx
 * make
 * sudo make install
 ******************************************************/

bool CallService::m_bRunning = false;
CallService *g_srv = nullptr;
CallService::CallService(/* args */)
{
    m_bRunning = false;
    g_srv = this;
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
    try
    {
        //       pqxx::connection conn("dbname=postgres user=postgres password=postgres hostaddr=192.168.1.92 port=5432");
        pqxx::connection conn("dbname=postgres user=postgres password=postgres hostaddr=127.0.0.1 port=5432");
        pqxx::work txn(conn);
        pqxx::result res = txn.exec("select version()");
        std::cout << "Server version: " << res[0][0].as<std::string>() << std::endl;

        std::string query = "SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_schema = 'public' AND table_name = 'calls_ex');";
        res = txn.exec(query);
        bool exists = res[0][0].as<bool>();
        if (exists)
        {
            std::cout << "Existed Table: " << "calls_ex" << std::endl;
        }
        else
        {
            std::cout << "None Existed Table: " << "calls_ex. It will be created..." << std::endl;
            std::string createTableQuery = "CREATE TABLE calls_ex(id SERIAL PRIMARY KEY, name VARCHAR(255));";
            txn.exec(createTableQuery);

            std::cout << "Created Table: " << "calls_ex" << std::endl;
        }
        txn.commit();
    }
    catch (const std::exception &e)
    {
        std::cout << "pg: " << e.what() << std::endl;
    }

    m_bRunning = true;

    std::thread{std::bind([this](int port)
                          {
        net::io_context ioc;
        WebsocketServer s(ioc,tcp::endpoint(net::ip::make_address("0.0.0.0"),port));
        ioc.run(); }, 18081)}
        .detach();
    std::thread{std::bind([this](int port)
                          {
        CRestfulServer server;
        server.runner(port,this); }, 18080)}
        .detach();

    esl_global_set_default_logger(0);
    std::thread{
        std::bind([this]()
                  { 
		  //esl_listen_threaded("192.168.1.91", 8040, CallService::callbackfun, this, 100000);
		  esl_listen_threaded("0.0.0.0", 18040, CallService::acd_callback, this, 100000); })}
        .detach();

    std::thread{
        std::bind([this]()
                  {
                      esl_handle_t handle = {0};
                      while (true)
                      {
                          if (esl_connect_timeout(&handle, "127.0.0.1", 8021, "", "ClueCon", 3000))
                          {
                              esl_log(ESL_LOG_ERROR, "Error Connecteling [%s]\n", handle.err);
                              continue;
                          }
                          break;
                      }

                      esl_thread_create_detached(CallService::eventThreadFun, &handle);
                      esl_events(&handle, ESL_EVENT_TYPE_PLAIN, "ALL");
                     // esl_events(&handle, ESL_EVENT_TYPE_PLAIN, "CHANNEL_HANGUP_COMPLETE");

		              this->setHandle(handle);
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
int CallService::makecall(std::string dn_, std::string dst_)
{
    // bgapi originate user/1001 015000415869 xml default
    auto it = std::find_if(getAgentList().begin(), getAgentList().end(), [dn_](std::shared_ptr<Agent> &agent_)
                           { return agent_->getDn() == dn_; });
    if (it == getAgentList().end())
    {
        return -1;
    }
    std::string command = "api uuid_answer " + (*it)->getUUID();
    esl_send_recv(getHandle(), command.c_str());
    if (getHandle()->last_sr_event && getHandle()->last_sr_event->body)
    {
        esl_log(ESL_LOG_INFO, "%s\n", getHandle()->last_sr_event->body);
    }

    return 0;
}
int CallService::answercall(std::string dn_)
{
    // bgapi uuid_answer 9c8a19e2-4ca7-402f-8324-75c76a0888a2
    return 0;
}
int CallService::hangupcall(std::string dn_)
{
    // bgapi uuid_kill 9c8a19e2-4ca7-402f-8324-75c76a0888a2
    return 0;
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
        {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
        break;
        case ESL_FAIL:
        {
            esl_log(ESL_LOG_WARNING, "Disconnected.\n");
        }
        break;
        case ESL_SUCCESS:
        {
            // esl_log(ESL_LOG_INFO, "coming event_body:%xs\n", handle->last_event);
            if (handle->last_event->body)
            {
                // esl_log(ESL_LOG_INFO, "event_body:%s\n", handle->last_event->body);
            }
            const char *cid_dest, *cid_number, *uuid, *cause, *hangup_disposition;
            cid_number = esl_event_get_header(handle->last_ievent, "Caller-Caller-ID-Number");
            cid_dest = esl_event_get_header(handle->last_ievent, "Caller-Destination-Number");
            cause = esl_event_get_header(handle->last_ievent, "Hangup-Cause");
            hangup_disposition = esl_event_get_header(handle->last_ievent, "variable_sip_hangup_disposition");
            uuid = esl_event_get_header(handle->last_ievent, "Unique-ID");
            char szMsg[1024] = {0};
            sprintf(szMsg, "UUID:%s Caller: %s Callee:%s HangupCause:%s hangup_disposition:%s", uuid, cid_number, cid_dest, cause, hangup_disposition);
            esl_event_t *pEvent = handle->last_ievent;
            switch (pEvent->event_id)
            {
            case ESL_EVENT_CUSTOM:
            {
            }
            break;
            case ESL_EVENT_RECORD_START:
            {
            }
            break;
            case ESL_EVENT_RECORD_STOP:
            {
            }
            break;
            case ESL_EVENT_CHANNEL_PROGRESS:
            {
                std::cout << "channel progress " << szMsg << std::endl;
            }
            break;
            case ESL_EVENT_CHANNEL_PROGRESS_MEDIA: // ring
            {
                std::cout << "channel progres media " << szMsg << std::endl;
            }
            break;
            case ESL_EVENT_CHANNEL_CREATE:
            {
                std::cout << "channel create " << szMsg << std::endl;
                auto it = std::find_if(g_srv->getAgentList().begin(), g_srv->getAgentList().end(), [cid_dest](std::shared_ptr<Agent> &agent_)
                                       { return agent_->getDn() == std::string(cid_dest); });
                if (it != g_srv->getAgentList().end())
                {
                    (*it)->setUUID(uuid);
                }
            }
            break;
            case ESL_EVENT_CHANNEL_ANSWER:
            {
                std::cout << "channel answer " << szMsg << std::endl;
            }
            break;
            case ESL_EVENT_CHANNEL_HANGUP:
            {
                std::cout << "channel hangup " << szMsg << std::endl;
            }
            break;
            case ESL_EVENT_CHANNEL_BRIDGE:
            {
                std::cout << "channel bridge " << szMsg << std::endl;
            }
            break;
            case ESL_EVENT_CHANNEL_HANGUP_COMPLETE:
            {
                std::cout << "channel hangup_complete " << szMsg << std::endl;
                if (hangup_disposition)
                {
                    if (std::string(hangup_disposition) == "recv_bye")
                    {
                        std::cout << "The caller hung up (Caller: " << cid_number << ", Callee: " << cid_dest << ")" << std::endl;
                    }
                    else if (std::string(hangup_disposition) == "send_bye")
                    {
                        std::cout << "The callee hung up (Caller: " << cid_number << ", Callee: " << cid_dest << ")" << std::endl;
                    }
                    else
                    {
                        std::cout << "Hangup cause: " << cause << std::endl;
                    }
                }
                else
                {
                    std::cout << "Hangup cause: " << cause << std::endl;
                }
                for (auto &agent_ : g_srv->getAgentList())
                {
                    if (agent_->getDn() == std::string(cid_dest))
                    {
                        agent_->setAgentStatus(AgentStatus_t::Ready);
                        agent_->setUUID("");
                        g_srv->printAgent(agent_);
                        break;
                    }
                }
            }
            break;
            case ESL_EVENT_CHANNEL_UNBRIDGE:
            {
            }
            break;
            case ESL_EVENT_CHANNEL_HOLD:
            {
            }
            break;
            case ESL_EVENT_CHANNEL_UNHOLD:
            {
            }
            break;
            case ESL_EVENT_DTMF:
            {
            }
            break;
            default:
                break;
            }
        }
        break;
        }
    }
    return nullptr;
}
void CallService::printAgent(std::shared_ptr<Agent> pAgent_)
{
    std::string state = pAgent_->getPolling() == true ? "True" : "False";
    std::cout << "dn:" << pAgent_->getDn() << " uuid:" << pAgent_->getUUID() << " Polling:" << state.c_str() << " status:" << (int)(pAgent_->getAgentStatus()) << std::endl;
}
void CallService::doService()
{
    while (m_bRunning)
    {
        if (m_queueEvent.empty())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(200));
            continue;
        }
        std::shared_ptr<CallEvent> pData = m_queueEvent.front();
        m_queueEvent.pop();

        std::string res, url, call_in_url = "", call_out_url = "";
        if (pData->getDir() == CallDir_t::callin)
        {
            url = call_in_url;
        }
        else
        {
            url = call_out_url;
        }

        int nRet = HttpPost(url.c_str(), pData->getData().c_str(), res, 0);
    }
}

void CallService::stopService()
{
    m_bRunning = false;
}

void CallService::acd_callback(esl_socket_t server_sock, esl_socket_t client_sock, sockaddr_in *addr, void *user_data)
{
    CallService *pThis = (CallService *)user_data;
    esl_handle_t handle = {{0}};
    esl_status_t status = ESL_SUCCESS;
    std::shared_ptr<Agent> pAgent = nullptr;
    esl_attach_handle(&handle, client_sock, addr);

    esl_log(ESL_LOG_INFO, "Connected! %d\n", handle.sock);
    const char *cid_name, *cid_number;
    cid_name = esl_event_get_header(handle.info_event, "Caller-Caller-ID-Name");
    cid_number = esl_event_get_header(handle.info_event, "Caller-Caller-ID-Number");
    esl_log(ESL_LOG_INFO, "New call from %s<%s>\n", cid_name, cid_number);

    esl_send_recv(&handle, "myevent");
    esl_log(ESL_LOG_INFO, "%s\n", handle.last_sr_reply);

    esl_send_recv(&handle, "linger 5");
    esl_log(ESL_LOG_INFO, "%s\n", handle.last_sr_reply);

    esl_execute(&handle, "answer", NULL, NULL);
    esl_execute(&handle, "set", "tts_engine=tts_commandline", NULL);
    esl_execute(&handle, "set", "tts_voice=Ting-ting", NULL);
    esl_execute(&handle, "set", "continue_on_fail=true", NULL);
    esl_execute(&handle, "set", "hangup_after_bridge=true", NULL);
    esl_execute(&handle, "speak", "您好，欢迎致电，电话接通中，请稍后", NULL);
    sleep(1);
    esl_execute(&handle, "playback", "local_stream://moh", NULL);

    while (status == ESL_SUCCESS || status == ESL_BREAK)
    {
        const char *type;
        const char *application;

        status = esl_recv_timed(&handle, 1000);
        if (status == ESL_BREAK)
        {
            pAgent = pThis->get_available_agent();
            if (pAgent != nullptr)
            {
                std::string routedAgent = "user/";
                routedAgent += pAgent->getDn();
                esl_execute(&handle, "break", NULL, NULL);
                esl_execute(&handle, "bridge", routedAgent.c_str(), NULL);
                esl_log(ESL_LOG_INFO, "Calling to:%s\n", routedAgent.c_str());
                break;
            }
            continue;
        }
        type = esl_event_get_header(handle.last_event, "content-type");
        if (type && !strcasecmp(type, "text/event-plain"))
        {
            const char *uuid = esl_event_get_header(handle.last_ievent, "Other-Leg-Unique-ID");
            switch (handle.last_ievent->event_id)
            {
            case ESL_EVENT_CHANNEL_BRIDGE:
                if (pAgent)
                {
                    pAgent->setUUID(uuid);
                }
                esl_log(ESL_LOG_INFO, "Bridged to:%s\n", uuid);
                break;
            case ESL_EVENT_CHANNEL_HANGUP_COMPLETE:
                if (pAgent)
                {
                    pThis->reset_agents(pAgent);
                    pAgent = nullptr;
                }
                esl_log(ESL_LOG_INFO, "Caller:%s<%s> Hangup\n", cid_name, cid_number);
                goto end;
            case ESL_EVENT_CHANNEL_EXECUTE_COMPLETE:
                application = esl_event_get_header(handle.last_ievent, "Application");
                if (!strcmp(application, "bridge"))
                {
                    const char *disposition = esl_event_get_header(handle.last_ievent, "variable_originate_disposition");
                    esl_log(ESL_LOG_INFO, "dispostion: %s\n", disposition);
                    if (!strcmp(disposition, "CALL_REJETED") || !strcmp(disposition, "USER_BUSY"))
                    {
                        pThis->reset_agents(pAgent);
                        pAgent = nullptr;
                    }
                }
                if (pAgent)
                {
                    pThis->reset_agents(pAgent);
                }
                esl_log(ESL_LOG_INFO, "CSPBX Caller:%s<%s> Hangup\n", cid_name, cid_number);
                break;
            default:
                break;
            }
        }
    }
end:
    esl_log(ESL_LOG_INFO, "Disconnectd!%d\n", handle.sock);
    esl_disconnect(&handle);
}

void CallService::reset_agents(std::shared_ptr<Agent> pAgent_)
{
    pAgent_->setPolling(false);
}
std::shared_ptr<Agent> CallService::get_available_agent()
{
    std::shared_ptr<Agent> pAgent = nullptr;

    auto it = std::find_if(getAgentList().begin(), getAgentList().end(), [](std::shared_ptr<Agent> &agent_)
                           { return agent_->getPolling() == false; });
    if (it == getAgentList().end())
    {
        std::for_each(getAgentList().begin(), getAgentList().end(), [](auto x)
                      { x->setPolling(false); });
    }

    for (auto &agent_ : getAgentList())
    {
        printAgent(agent_);

        if (agent_->getPolling() == false && agent_->getAgentStatus() == AgentStatus_t::Ready)
        {
            agent_->setPolling(true);
            pAgent = agent_;
            break;
        }
    }
    return pAgent;
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

            std::string routeAgent = "user/";
            routeAgent += agent_->getDn();

            esl_execute(&handle, "bridge", routeAgent.c_str(), NULL);
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
