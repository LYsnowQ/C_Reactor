#include "TcpConnection.h"
#include "Buffer.h"
#include "Channel.h"
#include "EventLoop.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Log.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int processWrite(void* arg)
{
    LOG_DEBUG("开始发送数据（基于写事件）....");
    struct TcpConnection* conn = (struct TcpConnection*)arg;

    int count = bufferSendData(conn->writeBuffer, conn->channel->fd);
    if(count > 0)
    {
        if(bufferReadableSize(conn->writeBuffer) == 0)
        {
            //writeEventEnable(conn->channel, false);

            //eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);

            eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
        }
    }

    return 0;
}

int processRead(void* arg)
{
    struct TcpConnection* conn = (struct TcpConnection*)arg;
    int count = bufferSocketRead(conn->readBuffer, conn->channel->fd); 
    
    LOG_DEBUG("接收到http请求，解析http请求：%s",conn->readBuffer->data+conn->readBuffer->readPos);

    if(count > 0)
    {
        //接收http请求，解析http请求
        int socket = conn->channel->fd;
#ifdef MSG_SEND_AUTO                
        writeEventEnable(conn->channel,true);
        eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
#endif
        bool flag = parseHttpRequest(conn->request, conn->readBuffer, conn->response, conn->writeBuffer,socket);
        if(!flag)
        {
            //解析失败
            char* errMsg = " Http/1.1 400 Bad Request\r\n\r\n";
            bufferAppendString(conn->writeBuffer, errMsg);
        }
    }
#ifndef MSG_SEND_AUTO
    //断开连接
    eventLoopAddTask(conn->evLoop, conn->channel ,DELETE);
#endif
    return 0;
}

struct TcpConnection* tcpConnectionInit(int fd,struct EventLoop* evLoop)
{   
    struct TcpConnection* conn = (struct TcpConnection*)malloc(sizeof(struct TcpConnection));
    conn->evLoop = evLoop;
    conn->readBuffer = bufferInit(10240);
    conn->writeBuffer = bufferInit(10240);
    
    // http
    conn->request = httpRequestInit();
    conn->response = httpResponseInit();
    sprintf(conn->name,"Connection-%d",fd);
    conn->channel = channelInit(fd ,ReadEvent, processRead, processWrite, tcpConnectionDestory,conn);
    eventLoopAddTask(evLoop, conn->channel, ADD);

    LOG_DEBUG("和客户端建立连接,threadName: %s, threadID: %ld,connName: %s",\
            evLoop->threadName,evLoop->threadID,conn->name);

    if(!conn->readBuffer || 
        !conn->writeBuffer ||
        !conn->channel ||
        !conn->evLoop)
    {
        free(conn);
        perror("TcpConnection");
        return NULL;
    }

    return conn;
}

int tcpConnectionDestory(void* arg)
{
    struct TcpConnection* conn = (struct TcpConnection*)arg;
    if(conn == NULL)
    {
        return 0;
    } 
    char name[32] = {0};
    strncpy(name, conn->name, sizeof(name) - 1);
    if(conn->readBuffer && bufferReadableSize(conn->readBuffer) == 0 &&
        conn->writeBuffer && bufferReadableSize(conn->writeBuffer) == 0)
    {
        destoryChannel(conn->evLoop, conn->channel);
        bufferDestory(conn->readBuffer);
        bufferDestory(conn->writeBuffer);
        httpRequestDestory(conn->request);
        httpResponseDestory(conn->response);
        free(conn);
    }
    LOG_DEBUG("连接断开，释放资源，connName:%s",name);
    return 0;
}
