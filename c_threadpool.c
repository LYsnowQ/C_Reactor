#include "c_threadpool.h"
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


const int NUMBER = 2;

//任务结构体

typedef struct Task
{
    void (*function)(void* arg);
    void* arg;
}Task;


struct ThreadPool
{
    //任务队列
    Task* taskQ;
    int queueCapacity;
    int queueSize;//current size
    int queueFront;
    int queueRear; 

    pthread_t manageerID; 
    pthread_t *threadIDs;
    int minNum;
    int maxNum;
    int busyNum;
    int liveNum;
    int exitNum;
    pthread_mutex_t mutexpool;//lock ThreadPool
    pthread_mutex_t mutexBusy;//lock BusyNun
    pthread_cond_t notFull;
    pthread_cond_t notEmpty;

    int shutdown;//destory:1 not do:0
};

ThreadPool* threadPoolCreate(int min,int max,int queueSize)
{
    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    do{
        if(pool == NULL)
        {
            printf("malloc threadpool fail...\n");
            break;
        }

        pool->threadIDs = (pthread_t*)malloc(sizeof(pthread_t) * max);
        if(pool->threadIDs == NULL)
        {
            printf("malloc threadIDs fail...\n");
            break;
        }
        memset(pool->threadIDs,0,sizeof(pthread_t));
        pool -> minNum = min;
        pool -> maxNum = max; 
        pool -> busyNum = 0;
        pool -> liveNum = min;
        pool -> exitNum = 0;

        if(pthread_mutex_init(&pool->mutexpool,NULL)!=0 ||
                pthread_mutex_init(&pool->mutexBusy,NULL)!=0||
                pthread_cond_init(&pool->notFull,NULL)!=0||
                pthread_cond_init(&pool->notEmpty,NULL)!=0)
        {
            printf("mutex or condition init fail...\n");
            break;
        }

        pool -> taskQ = (Task*)malloc(sizeof(Task)*queueSize);
        pool -> queueCapacity = queueSize;
        pool -> queueSize = 0;
        pool -> queueFront = 0;
        pool -> queueRear = 0;
        pool -> shutdown = 0;

        pthread_create(&pool->manageerID,NULL,manager,pool); 
        for(int i = 0; i < min ; i++ )
            pthread_create(&pool->threadIDs[i],NULL,worker,pool);
        return pool;
    }while(0);
    
    //free sources
    if(pool && pool->taskQ)free(pool->taskQ);
    if(pool && pool->threadIDs)free(pool->threadIDs);
    if(pool)free(pool);

    return NULL;
}


void* worker(void* arg)
{
    ThreadPool* pool = (ThreadPool*)arg;

    while(1)
    {
        pthread_mutex_lock(&pool->mutexpool);
        while(pool->queueSize == 0 && !pool->shutdown)
        {
            
            pthread_cond_wait(&pool->notEmpty,&pool->mutexpool);
            if(pool->exitNum > 0)
            {
                pool->exitNum--;
                pthread_mutex_unlock(&pool->mutexpool);
                pthread_exit(NULL);
            }
        }
        if(pool->shutdown)
        {
            pool->exitNum--;
            pool->liveNum--;
            pthread_mutex_unlock(&pool->mutexpool);
            threadExit(pool);
        }
        Task task;
        task.function = pool->taskQ[pool->queueFront].function;
        task.arg = pool->taskQ[pool->queueFront].arg;

        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        pool->queueSize--;
        
        pthread_cond_signal(&pool->notFull);
        pthread_mutex_unlock(&pool->mutexpool);

        printf("thread %ld start working...\n",pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);

        task.function(task.arg);
        free(task.arg);
        task.arg = NULL;

        printf("thread %ld end working...\n",pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);
    }

    return NULL;
}


void* manager(void* arg)
{
    ThreadPool* pool = (ThreadPool*)arg; 
    while(!pool->shutdown)
    {
        sleep(3);
        
        pthread_mutex_lock(&pool->mutexpool);
        int queueSize = pool->queueSize;
        int liveNum = pool->liveNum;
        pthread_mutex_unlock(&pool->mutexpool);

        pthread_mutex_lock(&pool->mutexBusy);
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&pool->mutexBusy);
    
        if(queueSize > liveNum && liveNum < pool->maxNum)
        {
            pthread_mutex_lock(&pool->mutexpool);
            int counter = 0;
            for(int i = 0; i < pool->maxNum && 
                  counter<NUMBER && 
                  counter < NUMBER && 
                  pool->liveNum<pool->maxNum;i++)
                if(pool -> threadIDs[i] == 0)
                {
                    pthread_create(&pool->threadIDs[i], NULL, worker, pool);
                    counter++;
                    pool->liveNum++;
                }
            pthread_mutex_unlock(&pool->mutexpool);
        }

        if(busyNum * 2 < liveNum && liveNum > pool->minNum)
        {
            pthread_mutex_lock(&pool->mutexpool);
            pool->exitNum = NUMBER;
            pthread_mutex_unlock(&pool->mutexpool);
            for(int i = 0;i<NUMBER;i++)
            {
                pthread_cond_signal(&pool -> notEmpty);
            }
        }
    }

    return NULL;
}

void threadExit(ThreadPool* pool)
{
    pthread_t tid = pthread_self();
    for(int i =0;i < pool->maxNum; i++)
    {
        if(pool->threadIDs[i] == tid)
        {
            pool -> threadIDs[i] = 0;
            printf("threadExit called ,%ld exiting...\n",tid);
            break;
        }
    }
    pthread_exit(NULL);
}



void threadPoolAdd(ThreadPool* pool,void(*func)(void*),void* arg)
{
    pthread_mutex_lock(&pool->mutexpool);
    while(pool->queueSize == pool->queueCapacity && !pool->shutdown)
    {
        pthread_cond_wait(&pool->notFull, &pool->mutexpool);
    }
    if(pool->shutdown)
    {
        pthread_mutex_unlock(&pool->mutexpool);
        return;
    }

    pool->taskQ[pool->queueRear].function = func;
    pool->taskQ[pool->queueRear].arg = arg;
        
    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    pool->queueSize++;

    pthread_cond_signal(&pool->notEmpty);

    pthread_mutex_unlock(&pool->mutexpool);

}


int threadPoolBusyNum(ThreadPool* pool)
{
    pthread_mutex_lock(&pool -> mutexBusy);
    int busyNum = pool -> busyNum;
    pthread_mutex_unlock(&pool -> mutexBusy);
    return busyNum;
}

int threadPoolAlibeNum(ThreadPool* pool)
{
    pthread_mutex_lock(&pool -> mutexpool);
    int aliveNum = pool->liveNum;
    pthread_mutex_unlock(&pool->mutexpool);
    return aliveNum;
}

int threadPoolDestory(ThreadPool* pool)
{
    if(pool == NULL)
    {
        return -1;
    }
    
    pool->shutdown = 1;
    pthread_join(pool->manageerID, NULL);
    for(int i = 0 ; i < pool->liveNum;i++)
    {
        pthread_cond_signal(&pool->notEmpty);
    }
    
    if(pool->taskQ)
    {
        free(pool->taskQ);
        pool->taskQ = NULL;
    }
    if(pool->threadIDs)
    {
        free(pool->threadIDs);
        pool->threadIDs = NULL;
    }
    pthread_mutex_destroy(&pool->mutexpool);
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_cond_destroy(&pool->notFull);
    pthread_cond_destroy(&pool->notEmpty);
    
    free(pool);
    pool = NULL;
    printf("ThreadPool has been destoriesd...");    
    return 0;
}
