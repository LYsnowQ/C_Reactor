#pragma once

struct Buffer
{
    char* data;
    int capacity;
    int readPos;
    int writePos;
};

struct Buffer* bufferInit(int size);

void bufferDestory(struct Buffer* buf);

void bufferExtendRoom(struct Buffer* buffer,int size);

int bufferWriteableSize(struct Buffer* buffer);

int bufferReadableSize(struct Buffer* buffer);

int bufferAppendData(struct Buffer* buffer,const char* data,int size);

int bufferAppendString(struct Buffer* buffer,const char* data);

int bufferSocketRead(struct Buffer* buffer,int fd);

int bufferSocketWrite(struct Buffer* buffer,int fd);

int bufferSendData(struct Buffer* buffer, int socket);
