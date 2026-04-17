#pragma once
#include <pthread.h>
#include "EventLoop.h"

struct WorkerThread
{
    pthread_t threadID;
    char name[24];
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    struct EventLoop* evLoop; // 反应堆
};

int workerThreadInit(struct WorkerThread* thread,int index);

void workerThreadRun(struct WorkerThread* thread);
