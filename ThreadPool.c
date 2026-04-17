#include "ThreadPool.h"
#include "EventLoop.h"
#include "WorkerThread.h"
#include "Log.h"

#include <assert.h>


struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop,int count)
{
    LOG_DEBUG("初始化线程池...");
    struct ThreadPool* pool = (struct ThreadPool*)malloc(sizeof(struct ThreadPool));
    pool->index = 0;
    pool->isStart = false;
    pool->mainLoop = mainLoop;
    pool->threadNum = count;
    pool->workerThreads = (struct WorkerThread*)malloc(sizeof(struct WorkerThread)* count);
    return pool;
}

void threadPoolRun(struct ThreadPool* pool)
{
    LOG_DEBUG("线程池启动...");
    assert(pool && !pool->isStart);
    if(pool->mainLoop->threadID != pthread_self())
    {
        exit(0);
    }
    pool->isStart = true;
    
    LOG_DEBUG("启动工作线程...");
    if(pool->threadNum)
    {
        for(int i = 0;i < pool->threadNum; i++)
        {
            workerThreadInit(&pool->workerThreads[i], i);
            workerThreadRun(&pool->workerThreads[i]);
        }
    }
    LOG_DEBUG("线程池启动完成...");
}


struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool)
{
    assert(pool->isStart);

    struct EventLoop* evLoop = pool->mainLoop;
    if(pool->threadNum > 0) 
    {
        evLoop = pool->workerThreads[pool->index].evLoop;
        pool->index = (pool->index + 1) % pool->threadNum;
    }
    return evLoop;
} 
