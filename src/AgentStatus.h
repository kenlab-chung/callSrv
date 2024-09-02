/*************************************************************************
  > File Name: AgentStatus.h
  > Author: zhongqf
  > Mail: zhongqf.cn@gmail.com 
  > Created Time: 2024-08-28
 ************************************************************************/
enum class AgentStatus : unsigned
{
    unknown = 0,
    login = 101,
    logout = 102,
    notReady = 103,
    Ready =104,
    busy = 105,
    idle =106,
    leave = 107
};