#include "EventLoop.h"
#include "Channel.h"

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


struct EventLoop* eventLoopInit()
{
    return eventLoopInitEx(NULL);
}

// 写数据
void taskWakeup(struct EventLoop* evLoop)
{
    const char* msg = "wakeUp!";
    write(evLoop->socketPair[0],msg,strlen(msg));
}

// 读数据
int readLocalMessage(void* arg)
{
    struct EventLoop* evLoop = (struct EventLoop*)arg;
    char buf[256];
    read(evLoop->socketPair[1],buf,sizeof(buf));
    return 0;
}

struct EventLoop* eventLoopInitEx(const char* threadName)
{
    struct EventLoop* evLoop = (struct EventLoop*)malloc(sizeof(struct EventLoop));
    evLoop->isQuit = false;
    evLoop->head = NULL;
    evLoop->tail = NULL;
    
    evLoop->threadID = pthread_self();
    
    pthread_mutex_init(&evLoop->mutex, NULL);
    
    strcpy(evLoop->threadName,(!threadName || threadName[0]=='\0')? "MainThread" : threadName);
    
    evLoop->dispatcher = &PollDispatcher;
    evLoop->dispatcherData = evLoop->dispatcher->init();

    evLoop->channelMap = channelMapInit(128);
    
    //规则：evLoop->socketPair[0]发送数据 evLoop->socketPair[1]接收数据    
    int ret = socketpair(AF_UNIX,SOCK_STREAM,0,evLoop->socketPair);
    if(ret == -1)
    {
        perror("socketpair");
        exit(0);
    }

    struct Channel* channel = channelInit(evLoop->socketPair[1],ReadEvent,readLocalMessage,NULL,NULL,evLoop);
    eventLoopAddTask(evLoop, channel, ADD);

     
    return evLoop;
}


int eventLoopRun(struct EventLoop* evLoop)
{
    assert(evLoop != NULL);

    struct Dispatcher* dispatcher = evLoop->dispatcher;

    if(pthread_self() != evLoop->threadID)
    {
        return -1;
    }

    while(!evLoop->isQuit)
    {
        dispatcher->dispatch(evLoop,2);
        eventLoopProcessTask(evLoop);
    }

    return 0;

}


int eventActivate(struct EventLoop* evLoop,int fd,int event)
{
    if(fd < 0 || evLoop == NULL)
    {
        return -1;
    }

    struct Channel* channel = evLoop->channelMap->list[fd];
    assert(fd == channel->fd);

    if(event & ReadEvent && channel->readCallback)
    {
        channel->readCallback(channel->arg);
    }

    if(event & WriteEvent && channel->writeCallback)
    {
        channel->writeCallback(channel->arg);
    }

    return 0;
}

int eventLoopAddTask(struct EventLoop* evLoop,struct Channel* channel,int type)
{
    pthread_mutex_lock(&evLoop->mutex);
    
    struct ChannelElement* node = (struct ChannelElement*)malloc(sizeof(struct ChannelElement));
    node->channel = channel;
    node->type = type;
    node->next = NULL;
    
    if(evLoop->head == NULL)
    {
        evLoop->head = evLoop->tail = node;
    }
    else 
    {
        evLoop->tail->next = node;
        evLoop->tail = node;   
    }
    pthread_mutex_unlock(&evLoop->mutex);
    // 细节处理:
    // 1. 对于往链表中新增节点，可能由当前线程处理，也可能由主线程处理。
    //   1) 修改 fd 事件的操作，可能由当前线程发起，也由当前线程处理。
    //   2) 新增 fd 的操作由主线程发起。
    // 2. 不应让主线程去处理本该由子线程处理的任务队列。

    if(evLoop->threadID == pthread_self())
    {
        // 当前线程是子线程
        eventLoopProcessTask(evLoop);
    }
    else 
    {
        // 当前线程是主线程 -- 通知子线程处理任务队列中的任务
        // 此时子线程有两种可能:
        // 1. 子线程正在工作。2. 子线程阻塞在 select/poll/epoll 上
        taskWakeup(evLoop); 
    }
    return 0;
}


int eventLoopProcessTask(struct EventLoop *evLoop)
{
    pthread_mutex_lock(&evLoop->mutex);
    
    struct ChannelElement* head = evLoop->head;
    while(head!=NULL)
    {
        struct Channel* channel = head->channel;
        if(head->type == ADD)
        {
            eventLoopAdd(evLoop, channel);             
        }
        else if(head->type == DELETE)
        {
            eventLoopRemove(evLoop, channel); 
        }
        else if(head->type == MODIFY)
        {
            eventLoopModify(evLoop, channel);
        }
        
        struct ChannelElement* temp = head;
        head = head->next;
        free(temp);
    }
    evLoop->head = evLoop->tail = NULL;
    pthread_mutex_unlock(&evLoop->mutex);
    return 0;
}

int eventLoopAdd(struct EventLoop* evLoop,struct Channel* channel)
{
    int fd = channel -> fd;
    struct ChannelMap* channelMap = evLoop->channelMap;
    if(fd >= channelMap->size)
    {
        // 空间不够
        if(!makeMapRoom(channelMap, fd , sizeof(struct Channel*)))
        {
            return -1;
        }
    }
    
    if(channelMap->list[fd] == NULL)
    {
        channelMap->list[fd] = channel;
        evLoop->dispatcher->add(channel,evLoop);
    }

    return 0;
}


int eventLoopRemove(struct EventLoop* evLoop,struct Channel* channel)
{
    int fd = channel-> fd;
    struct ChannelMap* channelMap = evLoop->channelMap;
    if(fd >= channelMap->size)
    {
        return -1;
    }

    int ret = evLoop->dispatcher->remove(channel,evLoop);

    return ret;
}


int eventLoopModify(struct EventLoop* evLoop,struct Channel* channel)
{
    int fd = channel-> fd;
    struct ChannelMap* channelMap = evLoop->channelMap;
    if(fd >= channelMap->size || channelMap->list[fd] ==NULL)
    {
        return -1;
    }

    int ret = evLoop->dispatcher->modify(channel,evLoop);
    return ret;

}

int destoryChannel(struct EventLoop *evLoop, struct Channel *channel)
{
    evLoop->channelMap->list[channel->fd] = NULL;
    close(channel->fd);
    free(channel);
    return 0;
}
