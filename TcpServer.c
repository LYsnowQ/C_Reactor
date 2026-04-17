
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>

#include "TcpServer.h"
#include "EventLoop.h"
#include "ThreadPool.h"
#include "TcpConnection.h"
#include "Log.h"

struct Listener* listenerInit(unsigned short port)
{
    LOG_DEBUG("开始初始化监听端口...");
    int lfd = socket(AF_INET,SOCK_STREAM,0);
    if(lfd==-1)
    {
        perror("socket");
        return NULL;
    }

    int opt = 1;
    int ret = setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    if(ret == -1)
    {
        perror("setsockopt");
        return NULL;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(lfd,(struct sockaddr*)&addr,sizeof(addr));
    if(ret == -1)
    {
        perror("bind");
        return NULL;
    }

    ret = listen(lfd,128);
    if (ret == -1)
    {
        perror("listen");
        return NULL;
    } 
    struct Listener* listener = (struct Listener*)malloc(sizeof(struct Listener));
    listener ->lfd = lfd;
    listener ->port = port;
    
    LOG_DEBUG("监听端口初始化完成");

    return listener;
}

struct TcpServer* tcpServerInit(int port,int threadNum)
{
    LOG_DEBUG("开始初始化tcpserver...");
    struct TcpServer* tcp = (struct TcpServer*)malloc(sizeof(struct TcpServer));
    if(tcp == NULL)
    {
        perror("malloc");
        return NULL;
    }
    tcp->listener = listenerInit(port);
    if(!tcp->listener)
    {
        free(tcp);
        perror("tcpServer");
        return NULL;
    }
    tcp->mainLoop = eventLoopInit();
    tcp->threadNum = threadNum;
    tcp->threadPool = threadPoolInit(tcp->mainLoop,threadNum);
    if(!tcp->listener || !tcp->mainLoop || !tcp->threadPool)
    {
        free(tcp);
        perror("tcpServer");
        return NULL;
    }
    LOG_DEBUG("tcpServer初始化完成");
    return tcp;
}


int acceptConnection(void* arg)
{
    struct TcpServer* server = (struct TcpServer*)arg;
    int cfd = accept(server->listener->lfd,NULL,NULL);
    
    struct EventLoop* evLoop =  takeWorkerEventLoop(server->threadPool);
    tcpConnectionInit(cfd, evLoop);
    return 0;
}


void tcpServerRun(struct TcpServer* server)
{
    if(server == NULL || server->listener == NULL || server->mainLoop == NULL || server->threadPool == NULL)
    {
        LOG_DEBUG("tcpServer启动失败: 参数无效");
        return;
    }
    LOG_DEBUG("tcpServer开始启动...");
    threadPoolRun(server->threadPool);
        
    struct Channel* channel = channelInit(server->listener->lfd, ReadEvent, acceptConnection,NULL, NULL,server);
    eventLoopAddTask(server->mainLoop, channel, ADD);
    
    eventLoopRun(server->mainLoop);

    LOG_DEBUG("服务器程序已启动");
}
