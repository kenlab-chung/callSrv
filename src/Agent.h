/*************************************************************************
  > File Name: Agent.h
  > Author: zhongqf
  > Mail: zhongqf.cn@gmail.com
  > Created Time: 2024-08-28
 ************************************************************************/
#pragma once
#include <string>
#include "AgentStatus.h"
class Agent
{
public:
    Agent()
    {
        m_strDN = "";
        m_strAgent = "";
        m_bPolling = false;
	m_strUUID="";
        m_eStatus = AgentStatus_t::unknown;
    }
    ~Agent() {}
    void setDn(std::string dn) { m_strDN = dn; }
    void setAgent(std::string agent) { m_strAgent = agent; }
    void setPolling(bool bPolling) { m_bPolling = bPolling; }
    void setAgentStatus(AgentStatus_t status_) { m_eStatus = status_; }
    void setUUID(std::string uuid) { m_strUUID = uuid; }

    std::string &getDn() { return m_strDN; }
    std::string &getAgent() { return m_strAgent; }
    bool getPolling() { return m_bPolling; }
    AgentStatus_t getAgentStatus() { return m_eStatus; }
    std::string &getUUID(){ return m_strUUID; }

private:
    std::string m_strDN;
    std::string m_strAgent;
    std::string m_strUUID;
    bool m_bPolling;
    AgentStatus_t m_eStatus;
};
