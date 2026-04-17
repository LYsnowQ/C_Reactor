#pragma once
#include <stdbool.h>


struct ChannelMap
{
    int size;
    //struct Channel* list[]
    struct Channel** list;  
};

struct ChannelMap* channelMapInit(int size);

void ChannelMapClear(struct ChannelMap* map);

bool makeMapRoom(struct ChannelMap* map,int newSize,int unitSize);
