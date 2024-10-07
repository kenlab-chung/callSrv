/*************************************************************************
  > File Name: main.h
  > Author: zhongqf
  > Mail: zhongqf.cn@gmail.com 
  > Created Time: 2024-08-28
 ************************************************************************/

#include <iostream>
#include <bits/stdc++.h>
#include <string>
#include "CallService.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>


const char *g_strVersion = "callService 1.0.0";
const char *g_configFilename = "/etc/callService.conf";

CallService g_cs;

void InitHandlers()
{

}

void InitDeamon(const  char* pName)
{
    int i;
    pid_t pid;
    if((pid=fork())!=0)
    {
        exit(0);
    }
    setsid();
    signal(SIGHUP,SIG_IGN);
    if((pid=fork())!=0)
    {
        exit(0);
    }
    chdir("/");
    umask(0);
again:
    if((pid=fork())!=0)
    {
        if(waitpid(pid,nullptr,0)!=pid)
        {
            exit(0);
        }
        usleep(1000*1000*3);
        goto again;
    }

}
int main(int argc,char* argv[],char* envp[])
{
   // freopen("esl_log.log", "a", stdout);
   // freopen("esl_log.log", "a", stderr);
    char config[260]={0};
    if(argc>1 && strcmp(argv[1],"-v")==0)
    {
        std::cout<<g_strVersion<<std::endl;
        std::cout<<__DATE__<<" "<<__TIME__<<std::endl;
        return 0;
    }
    InitHandlers();
    //InitDeamon(argv[0]); 

    if(g_cs.loadconfig()!=0)
    {
        std::cout<<"load config:"<<config<<" error."<<std::endl;
        return -1;
    }
    if(!g_cs.startUp())
    {
        std::cout<<"callService startup error."<<std::endl;
    }
    else{
        std::cout<<"callService startup successfully."<<std::endl;
        g_cs.doService();
    }
    g_cs.stopService();
    return 0;
}
