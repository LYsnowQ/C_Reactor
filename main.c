#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>


#include "c_threadpool.h"
#include "server.h"


int main (int argc,char* argv[]){

#if 1    
    
    if(argc < 3)
    {
        printf("./a.out port path\n");
        return -1;
    }

    unsigned short port = atoi(argv[1]);

    //切换服务器的工作路径
    chdir(argv[2]);

    //初始化套接字
    int lfd = initListenFd(port);
   
    //启动服务程序
    epollRun(lfd);    
#endif 

    return 0;
}
