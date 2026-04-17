#pragma once

#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <sys/socket.h>

#include "Dispatcher.h"
#include "ChannelMap.h"

extern struct Dispatcher EpollDispatcher;
extern struct Dispatcher PollDispatcher;
extern struct Dispatcher SelectDispatcher;


enum ChannelType
{
    ADD,
    DELETE,
    MODIFY
};

struct Dispatcher;
struct ChannelElement{
    
    int type;
    struct Channel* channel;
    struct ChannelElement* next;
};

struct EventLoop
{
    bool isQuit;
    struct Dispatcher* dispatcher;
    void* dispatcherData;
    
    // task Queue
    struct ChannelElement* head;
    struct ChannelElement* tail;

    //map name mutex
    struct ChannelMap* channelMap;
    pthread_t threadID;
    char threadName[32];
    pthread_mutex_t mutex;
      
    int socketPair[2]; //store loack communication fd that is initialized by socketpair
};


struct EventLoop* eventLoopInit();
struct EventLoop* eventLoopInitEx(const char* threadName);

int eventLoopRun(struct EventLoop* evLoop);

int eventActivate(struct EventLoop* evLoop,int fd,int event);

int eventLoopAddTask(struct EventLoop* evLoop,struct Channel* channel,int type);

int eventLoopProcessTask(struct EventLoop* evLoop);


int eventLoopAdd(struct EventLoop* evLoop,struct Channel* channel);

int eventLoopRemove(struct EventLoop* evLoop,struct Channel* channel);

int eventLoopModify(struct EventLoop* evLoop,struct Channel* channel);

int destoryChannel(struct EventLoop* evLoop,struct Channel* channel);
