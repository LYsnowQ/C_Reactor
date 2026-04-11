#pragma once
#include "Channel.h"
#include "EventLoop.h"

struct Dispatcher
{
    //init select/pool/epoll's datablock
    void* (*init)();    
    int (*add)(struct Channel* channel,struct EventLoop* evLoop);
    int (*remove)(struct Channel* channel,struct EventLoop* evLoop);
    int (*modify)(struct Channel* channel,struct EventLoop* evLoop);
    int (*dispatch)(struct EventLoop* evLoop,int timeout);//timeout:seconds
    int (*clear)(struct EventLoop* evLoop);
};
