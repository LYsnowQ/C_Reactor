#pragma once

#include <stdbool.h>

typedef int (*handleFunc)(void* arg);

struct Channel
{
    int fd;
    int events;

    handleFunc readCallback;
    handleFunc writeCallback;
    handleFunc destoryCallback;
    void* arg;
};

enum FDEvent
{
    TimeOut = 0x01,
    ReadEvent = 0x02,
    WriteEvent = 0x04
};


// 初始化一个 Channel
struct Channel* channelInit(int fd,int events,handleFunc readFunc ,handleFunc writeFunc,handleFunc destoryFunc, void* arg);

void writeEventEnable(struct Channel* channel,bool flag);

bool isWriteEventEnable(struct Channel* channel);

