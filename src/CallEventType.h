/*************************************************************************
  > File Name: CallEventType.h
  > Author: zhongqf
  > Mail: zhongqf.cn@gmail.com
  > Created Time: 2024-08-28
 ************************************************************************/
enum class CallEventType_t : unsigned
{
    Service_Initiated,
    Originated,
    Delivered,   
    Established,
    Connection_Cleared,
    Failed,
    Held,
    Retrieved,
    Queued,
    Diverted,
    Transfferred,
    Conferenced,
    Agent_Ready,
    Agent_NotReady
};