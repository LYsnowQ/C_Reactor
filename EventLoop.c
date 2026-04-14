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

//Write data
void taskWakeup(struct EventLoop* evLoop)
{
    const char* msg = "wakeUp!";
    write(evLoop->socketPair[0],msg,strlen(msg));
}

//Read data
void readLocalMessage(void* arg)
{
    struct EventLoop* evLoop = (struct EventLoop*)arg;
    char buf[256];
    read(evLoop->socketPair[1],buf,sizeof(buf));
}

struct EventLoop* eventLoopInitEx(const char* threadName)
{
    struct EventLoop* evLoop = (struct EventLoop*)malloc(sizeof(struct EventLoop));
    evLoop->isQuit = false;
    
    evLoop->threadID = pthread_self();
    
    pthread_mutex_init(&evLoop->mutex, NULL);
    
    strcpy(evLoop->threadName,(!threadName || threadName[0]=='\0')? "MainThread" : threadName);
    
    evLoop->dispatcher = &EpollDispatcher;
    evLoop->dispatcherData = evLoop->dispatcher->init();

    evLoop->channelMap = channelMapInit(128);
    
    int ret = socketpair(AF_UNIX,SOCK_STREAM,0,evLoop->socketPair);
    if(ret == -1)
    {
        perror("socketpair");
        exit(0);
    }

    struct Channel* channel = channelInit(evLoop->socketPair[1],ReadEvent,readLocalMessage,NULL,evLoop);
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
    //detail process:
    //1.for adding linked list node,it can be processed by current thread or main thread:
    //  1).the event of changing fd,it might be initiated by the current thread,and be processed by current too.
    //  2).the operation of adding new fd is initiated by main thread.
    //2.should not let main thread process task queue that it should be processed by child thread!!!

    if(evLoop->threadID == pthread_self())
    {
        //Current thread is child thread
        eventLoopProcessTask(evLoop);
    }
    else 
    {
        //Current thread is main thread -- tell child thread to process TaskQueue's Task
        //this time the child thread has two possiblities:
        //1.The child thread is working. 2.The child thread is blocked(select/poop/epoll)
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
            eventLoopAdd(evLoop, channel, ADD);             
        }
        else if(head->type == DELETE)
        {
            eventLoopRemove(evLoop, channel, DELETE); 
        }
        else if(head->type == MODIFY)
        {
            eventLoopModify(evLoop, channel, DELETE);
        }
        
        struct ChannelElement* temp = head;
        head = head->next;
        free(temp);
    }
    evLoop->head = evLoop->tail = NULL;
    pthread_mutex_unlock(&evLoop->mutex);
    return 0;
}

int eventLoopAdd(struct EventLoop* evLoop,struct Channel* channel,int type)
{
    int fd = channel -> fd;
    struct ChannelMap* channelMap = evLoop->channelMap;
    if(fd >= channelMap->size)
    {
        //don't have enough space
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


int eventLoopRemove(struct EventLoop* evLoop,struct Channel* channel,int type)
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


int eventLoopModify(struct EventLoop* evLoop,struct Channel* channel,int type)
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
