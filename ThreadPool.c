#include "ThreadPool.h"
#include "EventLoop.h"
#include "WorkerThread.h"
#include <assert.h>


struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop,int count)
{
    struct ThreadPool* pool = (struct ThreadPool*)malloc(sizeof(struct ThreadPool));
    pool->index = 0;
    pool->isStart = false;
    pool->mainLoop = mainLoop;
    pool->workerThreads = (struct WorkerThread*)malloc(sizeof(struct WorkerThread)* count);
    return pool;
}

void threadPoolRun(struct ThreadPool* pool)
{
    assert(pool && !pool->isStart);
    if(pool->mainLoop->threadID != pthread_self())
    {
        exit(0);
    }
    pool->isStart = true;

    if(pool->threadNum)
    {
        for(int i = 0;i < pool->threadNum; i++)
        {
            workerThreadInit(&pool->workerThreads[i], i);
            workerThreadRun(&pool->workerThreads[i]);
        }
    }
}


struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool)
{
    assert(pool->isStart);
    if(pool->mainLoop->threadID == pthread_self())
    {
        exit(0);
    }
   
    struct EventLoop* evLoop = pool->mainLoop;
    if(pool->threadNum > 0) 
    {
        evLoop = pool->workerThreads[pool->index].evLoop;
    }

    pool->index = ++pool->index % pool->threadNum;
    return evLoop;
} 
