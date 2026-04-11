#pragma once


int initListenFd(unsigned short port);

int epollRun(int lfd);

void* acceptClient(void* arg);

void* recvHttpRequest(void* arg);

int parseRequestLine(const char* line, int cfd);

int sendFile(const char* filename,int cfd);

int sendHeadMsg(int cfd, int status, const char* descr,const char* type,int length);

const char* getFileContentType(const char* filename);

int sendDir(const char* dirname ,int cfd);

void decodeMsg(char* to,char* from);

int hexToDec(char c);
