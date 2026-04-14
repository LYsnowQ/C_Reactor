#include "Buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>

struct Buffer* bufferInit(int size)
{
    struct Buffer* buffer = (struct Buffer*)malloc(sizeof(struct Buffer));
    if(buffer != NULL)
    {
        buffer->data = (char*)malloc(size);
        buffer->capacity = size;
        buffer->writePos = buffer->readPos = 0;
        memset(buffer->data,0,size);
    }
    return buffer;
}

void bufferDestory(struct Buffer* buf)
{
    if(buf != NULL)
    {
        if(buf->data != NULL)
        {
            free(buf);
        }
    }
}


void bufferExtendRoom(struct Buffer* buffer,int size)
{
    //there're three bossibilities:
    if(bufferWriteableSize(buffer)>= size)
    {
    //  1.memory is sufficent - doesn't need to be expanded
        return;
    }
    else if(buffer->readPos + bufferWriteableSize(buffer) >= size)
    {
    //  2.memory needs to be merged - dosen't need to be expanded
        int readabl = bufferReadableSize(buffer);
        memcpy(buffer->data,buffer->data+buffer->readPos,readabl);
        buffer->readPos = 0;
        buffer->writePos = readabl;
    }
    else 
    {
    //  3.memory is not sufficent - need to be expanded
        void* temp = realloc(buffer->data,buffer->capacity+size);   
        if(temp == NULL)
        {
            return ;
        }
        memset(temp+buffer->capacity,0,size);
        buffer->data = temp;
        buffer->capacity += size; 
    }
}


int bufferWriteableSize(struct Buffer* buffer)
{
    return buffer->capacity - buffer->readPos;
}


int bufferReadableSize(struct Buffer* buffer)
{
    return buffer->writePos - buffer->readPos;
}


int bufferAppendData(struct Buffer* buffer,const char* data,int size)
{
    if(buffer == NULL || data == NULL || data<= 0)
    {
        return -1;
    }

    bufferExtendRoom(buffer, size);

    memcpy(buffer->data+buffer->writePos, data, size);
    buffer->writePos += size;

    return 0;
}


int bufferAppendString(struct Buffer* buffer,const char* data)
{
    int size = strlen(data);
    int ret = bufferAppendData(buffer, data, size);
    return 0;
}


int bufferSocketRead(struct Buffer* buffer,int fd)
{
    struct iovec vec[2];
    int writeable = bufferWriteableSize(buffer);
    vec[0].iov_base = buffer->data + buffer->writePos;
    vec[0].iov_len = writeable;

    char* tempbuf = (char*)malloc(40960);

    vec[1].iov_base = tempbuf;
    vec[1].iov_len = 40960;
    
    int result = readv(fd,vec,2);
    if(result == -1)
    {
        free(tempbuf);
        return -1;
    }
    else if(result <= writeable)
    {
        buffer->writePos += result;
    }
    else 
    {
        buffer->writePos = buffer->capacity;
        bufferAppendData(buffer,tempbuf,result-writeable);    
    }
    free(tempbuf);
    return 0;
}

int bufferSocketWrite(struct Buffer* buffer,int fd);
