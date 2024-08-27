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
    m_bRunning = true;

    std::thread{std::bind([this](int port)
                          {
        CRestfulServer server;
        server.runner(port,this); }, 8080)}
        .detach();
    return true;
}

void CallService::doService()
{
    esl_global_set_default_logger(7);
    esl_listen_threaded("192.168.1.91", 8040, CallService::callbackfun, nullptr, 100000);
}

void CallService::stopService()
{
    m_bRunning = false;
}

void CallService::callbackfun(esl_socket_t server_sock, esl_socket_t client_sock, sockaddr_in *addr, void *user_data)
{
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
    esl_execute(&handle, "bridge", "user/1001", NULL);
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
    CURL* curl = NULL;
	CURLcode res = CURLE_FAILED_INIT;
	curl = curl_easy_init();
	if (curl == NULL)
	{
		const char* szError = curl_easy_strerror(res);		
		return (int)CURLE_FAILED_INIT;
	}
	struct curl_slist* headerlist = NULL;
	headerlist = curl_slist_append(headerlist, "Content-Type:application/x-www-form-urlencoded;charset=UTF-8");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, param);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CallService::writedata);
	std::string response;
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (char*)&response);
	res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	{
		const char* szError = curl_easy_strerror(res);		
	}
	else {
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
