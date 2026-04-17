#pragma once

#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

//是否每次组装完后自动发送（非一次性发送）
//#define MSG_SEND_AUTO

struct TcpConnection
{
    char name[32];
    struct EventLoop* evLoop;
    struct Channel* channel;
    struct Buffer* readBuffer;
    struct Buffer* writeBuffer;

    //http协议
    struct HttpRequest* request;
    struct HttpResponse* response;
};

int processWrite(void* arg);

int processRead(void* arg);

struct TcpConnection* tcpConnectionInit(int fd,struct EventLoop* evLoop);

int tcpConnectionDestory(void* arg);

