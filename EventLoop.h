#pragma once
#include "Dispatcher.h"

extern struct Dispatcher EpollDispatcher;

struct EventLoop
{
    struct Dispatcher* dispatcher;
    void* dispatcherData;
};
