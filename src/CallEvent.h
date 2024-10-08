
/*************************************************************************
  > File Name: CallData.h
  > Author: zhongqf
  > Mail: zhongqf.cn@gmail.com
  > Created Time: 2024-08-28
 ************************************************************************/
#pragma once
#include <string>
#include "CallDir.h"
#include "CallEventType.h"
#include "CallCause.h"

class CallEvent
{
public:
	CallEvent() {}
	virtual ~CallEvent() {};

	void setDir(CallDir_t dir_) { m_eDir = dir_; }
	void setData(std::string data_) { m_strData = data_; }
	void setOtherParty(std::string otherParty_) { m_strOtherParty = otherParty_; }
	void setEventType(CallEventType_t eventType_) { m_eEvent = eventType_; }
	void setCallCause(CallCause_t cause_) { m_eCallCause = cause_; }

	CallDir_t getDir() { return m_eDir; }
	std::string &getData() { return m_strData; }
	std::string &getOtherParty() { return m_strOtherParty; }
	CallEventType_t getEventType() { return m_eEvent; }
	CallCause_t getCallCause() { return m_eCallCause; }

private:
	CallDir_t m_eDir;
	CallEventType_t m_eEvent;
	std::string m_strDn;
	std::string m_strData;
	std::string m_strOtherParty;
	CallCause_t m_eCallCause;
};
