#pragma once 

#include "EventLoop.h"

struct Listener
{
    int lfd;
    unsigned short port;
};

struct TcpServer
{
    int threadNum;
    struct EventLoop* mainLoop;
    struct ThreadPool* threadPool;
    struct Listener* listener;
};


struct Listener* listenerInit(unsigned short port);

struct TcpServer* tcpServerInit(int port,int threadNum);

void tcpServerRun(struct TcpServer* server);
