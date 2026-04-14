#pragma once

#include "EventLoop.h"
#include "WorkerThread.h"
#include <stdlib.h>
#include <stdbool.h>

struct ThreadPool
{
    bool isStart;
    struct EventLoop* mainLoop;
    int threadNum;
    struct WorkerThread* workerThreads;
    int index;
};


struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop,int count);

void threadPoolRun(struct ThreadPool* pool);

struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool);
