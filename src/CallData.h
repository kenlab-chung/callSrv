
/*************************************************************************
  > File Name: CallData.h
  > Author: zhongqf
  > Mail: zhongqf.cn@gmail.com
  > Created Time: 2024-08-28
 ************************************************************************/
#pragma once
#include <string>
#include "CallDir.h"
class CallData
{
public:
	CallData(){}
	virtual ~CallData(){};

	void setDir(CallDir dir_){m_eDir=dir_;}
	void setData(std::string data_){m_data = data_;}
	CallDir getDir(){return m_eDir;}
	std::string& getData(){return m_data;}
private:
	CallDir		m_eDir;
	std::string 	m_data;
};
