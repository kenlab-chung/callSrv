/*************************************************************************
  > File Name: AgentStatus.h
  > Author: zhongqf
  > Mail: zhongqf.cn@gmail.com
  > Created Time: 2024-08-28
 ************************************************************************/
enum class AgentStatus_t : unsigned
{
  unknown = 0,
  login,
  logout,
  notReady,
  Ready,
  busy,
  idle,
  leave
};