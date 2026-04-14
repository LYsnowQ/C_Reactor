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


struct TcpServer* tcpServerInit(int port,int threadNum);

struct Listener* listenerInit(unsigned short port);

void tcpServerRun(struct TcpServer* server);
