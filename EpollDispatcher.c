#include "Dispatcher.h"
#include "Channel.h"
#include "EventLoop.h"
#include <sys/epoll.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/unistd.h>

#define MAX 520


static void* epollInit();    
static int epollAdd(struct Channel* channel,struct EventLoop* evLoop);
static int epollRemove(struct Channel* channel,struct EventLoop* evLoop);
static int epollModify(struct Channel* channel,struct EventLoop* evLoop);
static int epollDispatcher(struct EventLoop* evLoop,int timeout);//timeout:seconds
static int epollClear(struct EventLoop* evLoop);
static int epollCtl(struct Channel* channel,struct EventLoop* evLoop,int op);


struct Dispatcher EpollDispatcher = {
    epollInit,
    epollAdd,
    epollRemove,
    epollModify,
    epollDispatcher,
    epollClear
};

struct EpollData
{
    int epfd;
    struct epoll_event* events;

};

static void* epollInit()
{
    struct EpollData* data = (struct EpollData*)malloc(sizeof(struct EpollData));
    data->epfd = epoll_create(1);
    if(data->epfd == -1)
    {
        perror("epoll_create");
        exit(0);
    }
    
    data->events = (struct epoll_event*)calloc(MAX,sizeof(struct epoll_event));

    return data;
}    

static int epollCtl(struct Channel* channel,struct EventLoop* evLoop,int op)
{
    struct EpollData* data = (struct EpollData*) evLoop->dispatcherData;
    struct epoll_event ev;
    ev.data.fd = channel->fd;
    int events = 0;
    if(channel->events & ReadEvent)
    {
        events |=EPOLLIN;
    }

    if(channel->events & WriteEvent)
    {
        events |=EPOLLOUT;
    }

    ev.events = events; 
    int ret = epoll_ctl(data->epfd,op,channel->fd,&ev);
    return ret;
}


static int epollAdd(struct Channel* channel,struct EventLoop* evLoop)
{
    int ret = epollCtl(channel, evLoop, EPOLL_CTL_ADD);
    if(ret == -1)
    {
        perror("epoll_ctl_add");
        exit(0);
    }
    return ret;
}


static int epollRemove(struct Channel* channel,struct EventLoop* evLoop)
{
    int ret = epollCtl(channel, evLoop, EPOLL_CTL_DEL);
    if(ret == -1)
    {
        perror("epoll_ctl_delete");
        exit(0);
    }
    return ret;
}


static int epollModify(struct Channel* channel,struct EventLoop* evLoop)
{
    int ret = epollCtl(channel, evLoop, EPOLL_CTL_MOD);
    if(ret == -1)
    {
        perror("epoll_ctl_modify");
        exit(0);
    }
    return ret;
}


static int epollDispatcher(struct EventLoop* evLoop,int timeout)//timeout:seconds
{
    struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
    int count = epoll_wait(data->epfd, data->events, MAX, timeout*1000);
    
    for(int i = 0; i < count ;i++)
    {
        int events = data -> events[i].events;
        int fd = data->events[i].data.fd;
        
        //client has been disconnected
        if(events & EPOLLERR || events & EPOLLHUP)
        {
            //epollReMove(Channel,evLoop);
            continue;
        }
        
        if(events & EPOLLIN)
        {
            eventActivate(evLoop, fd, ReadEvent);            
        }
        
        if(events & EPOLLOUT)
        {
            eventActivate(evLoop, fd, WriteEvent);
        }

    }

    return 0;
}


static int epollClear(struct EventLoop* evLoop)
{
    struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
    free(data->events);
    close(data->epfd);
    free(data);
    return 0;
}
