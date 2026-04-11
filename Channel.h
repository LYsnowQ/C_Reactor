#pragma once

#include <stdbool.h>

typedef int(*handleFunc)(void* arg);

struct Channel
{
    int fd;
    int events;

    handleFunc readCallback;
    handleFunc writeCallback;
    void* arg;
};

enum FDEvent
{
    TimeOut = 0x01,
    ReadEvent = 0x02,
    WriteEvent = 0x04
};


//init a Channel
struct Channel* channelInit(int fd,int events,handleFunc readFunc ,handleFunc writeFunc,void* arg);

void writeEventEnable(struct Channel* channel,bool flag);

bool isWriteEventEnable(struct Channel* channel);


