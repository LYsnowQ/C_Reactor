#pragma once
#include "Channel.h"
#include "EventLoop.h"


struct EventLoop;
struct Dispatcher
{
    // 初始化 select/poll/epoll 的数据块
    void* (*init)();    
    int (*add)(struct Channel* channel,struct EventLoop* evLoop);
    int (*remove)(struct Channel* channel,struct EventLoop* evLoop);
    int (*modify)(struct Channel* channel,struct EventLoop* evLoop);
    int (*dispatch)(struct EventLoop* evLoop,int timeout);//超时: 秒
    int (*clear)(struct EventLoop* evLoop);
};
