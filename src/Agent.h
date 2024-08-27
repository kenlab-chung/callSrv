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
        m_eStatus = AgentStatus::unknown;
    }
    ~Agent() {}
    void setDn(std::string dn) { m_strDN = dn; }
    void setAgent(std::string agent) { m_strAgent = agent; }
    void setPolling(bool bPolling) { m_bPolling = bPolling; }
    std::string &getDn() { return m_strDN; }
    std::string &getAgent() { return m_strAgent; }
    bool getPolling() { return m_bPolling; }

private:
    std::string m_strDN;
    std::string m_strAgent;
    bool m_bPolling;
    AgentStatus m_eStatus;
};