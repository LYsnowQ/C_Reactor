#include "TcpServer.h"
#include "EventLoop.h"
#include "ThreadPool.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>


struct TcpServer* tcpServerInit(int port,int threadNum)
{
    struct TcpServer* tcp = (struct TcpServer*)malloc(sizeof(struct TcpServer));
    tcp->listener = listenerInit(port);
    tcp->mainLoop = eventLoopInit();
    tcp->threadNum = threadNum;
    tcp->threadPool = threadPoolInit(tcp->mainLoop,threadNum);
    return tcp;
}

struct Listener* ListenerInit(unsigned short port)
{
    int lfd = socket(AF_INET,SOCK_STREAM,0);
    if(lfd==-1)
    {
        perror("setsockopt");
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

    return listener;
}

int acceptConnection(void* arg)
{
    struct TcpServer* server = (struct TcpServer*)arg;
    int cfd = accept(server->listener->lfd,NULL,NULL);
    
    struct EventLoop* evLoop =  takeWorkerEventLoop(server->mainLoop);
    return 0;
}


void tcpServerRun(struct TcpServer* server)
{
    threadPoolRun(server->threadPool);
    
    struct Channel* channel = channelInit(server->listener->lfd, ReadEvent, acceptConnection, NULL,server);
    eventLoopAddTask(server->mainLoop, channel, ADD);
    
    eventLoopRun(server->mainLoop);
}
