#include "WorkerThread.h"
#include "EventLoop.h"

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int workerThreadInit(struct WorkerThread* thread,int index)
{
    thread->evLoop = NULL;
    thread->threadID = 0;
    sprintf(thread->name,"SubThrad-%d",index);
    pthread_mutex_init(&thread->mutex,NULL);

    pthread_cond_init(&thread->cond, NULL);
    return 0;
}

void* subThreadRunning(void* arg)
{
    struct WorkerThread* thread = (struct WorkerThread*)arg;

    pthread_mutex_lock(&thread->mutex);

    thread->evLoop = eventLoopInitEx(thread->name);
    
    pthread_mutex_unlock(&thread->mutex);
    pthread_cond_signal(&thread->cond);

    eventLoopRun(thread->evLoop);
    return NULL;
}

void workerThreadRun(struct WorkerThread* thread)
{   
    pthread_create(&thread->threadID,NULL,subThreadRunning,thread);
    
    pthread_mutex_lock(&thread->mutex);
    while(thread->evLoop == NULL)
    {
        pthread_cond_wait(&thread->cond,&thread->mutex);

    }
    pthread_mutex_unlock(&thread->mutex);
}
