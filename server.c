#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/sendfile.h>
#include <dirent.h>
#include <ctype.h>
#include <pthread.h>

#include "server.h"


struct FdInfo{
    int epfd;
    int fd;
    pthread_t tid;
};


int initListenFd(unsigned short port){

    //1.创建监听套接字
     
    int lfd = socket(AF_INET,SOCK_STREAM,0);
    if (lfd == -1)
    {
        perror("socket");
        return -1;
    }
    //2.设置端口复用    
    int opt = 1;
    int ret = setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof opt);
    if(ret == -1)
    {
        perror("setsockopt");
        return -1;
    }
    //3.绑定端口
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(lfd,(struct sockaddr*)&addr,sizeof(addr));
    if (ret == -1)
    {
        perror("bind");
        return -1;
    }
   
    //4.设置监听(监听数字最大128超过了也是128)
    ret = listen(lfd,128);
    if (ret == -1)
    {
        perror("listen");
        return -1;
    }
        
    //5.返回fd
    return lfd;
}




int epollRun(int lfd)
{
    //1. 创建epoll实例(参数已经弃用只要大于0即可)
    int epfd = epoll_create(1);
    if(epfd == -1)
    {
        perror("epoll_create");
        return -1;
    }

    //2. fld上树
    struct epoll_event ev;
    ev.data.fd = lfd;
    ev.events = EPOLLIN;
    int ret = epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ev);
    if(ret == -1)
    {
        perror("epoll_ctl");
        return -1;
    }
    
    //3.检测
    struct epoll_event evs[1024];
    int ep_size = sizeof(evs)/sizeof(struct epoll_event);
    while(1)
    {
        int num = epoll_wait(epfd,evs,ep_size,-1); //为0时没有事件触发就会直接返回0为-1时没有事件触发会一直阻塞    
        for(int i=0;i<num;i++)
        {
            struct FdInfo* info = (struct FdInfo*)malloc(sizeof(struct FdInfo));
            
            int fd = evs[i].data.fd;
            
            info->epfd = epfd;
            info->fd = fd;

            if(fd == lfd)
            {
                //acceptClient(fd,epfd);
                pthread_create(&info->tid,NULL,acceptClient,info);
            }
            else
            {
                //recvHttpRequest(fd, epfd);
                pthread_create(&info->tid,NULL,recvHttpRequest,info);
            }
        }
    }

    return 0;
}


void* acceptClient(void* arg)
{
    struct FdInfo* info = (struct FdInfo*)arg;
    int lfd = info->fd;
    int epfd = info -> epfd;
    //建立链接
    printf("开始建立链接。。。\n");
    struct sockaddr caddr;
    socklen_t len = sizeof(caddr);
    int cfd = accept(lfd, &caddr, &len);
    if(cfd == -1)
    {
        perror("accept");
        return NULL;
    }
    printf("连接已建立。。。\n"); 
    //设置非阻塞
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd,F_SETFL,flag);

    //cfd添加到epoll中
    struct epoll_event ev;
    ev.data.fd = cfd;
    ev.events = EPOLLIN | EPOLLET;
    int ret = epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&ev);
    if(ret == -1)
    {
        perror("accept_epoll_ctl");
        return NULL;
    }        
    printf("cfd成功添加到epoll\n");
    free(info);
    return 0;
}


void* recvHttpRequest(void* arg)
{
    struct FdInfo* info = (struct FdInfo*)arg;
    int cfd = info -> fd;
    int epfd = info -> epfd;
    printf("开始接收http请求");
    int len = 0,total = 0;
    char temp[1024] = { 0 };
    char buff[4096] = { 0 };
    while((len = recv(cfd,temp,sizeof(temp),0)) > 0)
    {
        if(total+len < sizeof(buff))
        {
            memcpy(buff+total,temp,len);
            total +=len;
        }
    }
    
    //判断数据是接受完还是失败
    if(len == -1 && errno == EAGAIN)
    {
        //解析http请求
        printf("开始解析http请求。\n");
        char *pt =strstr(buff, "\r\n");
        int reqlen = pt - buff;
        buff[reqlen]='\0';
        parseRequestLine(buff,cfd);    
    }
    else if(len == 0)
    {
        //客户端已经断开了链接
        epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
        close(cfd);
        printf("客户端断开了连接。\n");
    }
    else
    {
        perror("recv");
    }
    free(info);
    return 0;
}


int parseRequestLine(const char *line, int cfd)
{
    //format:(get /xxx/1.jpg http/1.1)
    char method[12];
    char path[256];

    sscanf(line , "%[^ ] %[^ ]" ,method, path);
    
    if(strcasecmp(method,"get") != 0)
    {
        return -1;
    }
    
    decodeMsg(path, path);
    printf("解析http请求头method:%s,path:%s\n",method,path);

    //process client's static resource request
    char* file = NULL;
    if(strcmp(path,"/")==0)
    {
        file = "./";
    }
    else 
    {
        file = path + 1;
               
    }
    //get file's property
    struct stat st;
    int ret = stat(file, &st);
    if(ret == -1)
    {
        //file dosen't exist,response 404
        sendHeadMsg(cfd,404,"Not Found",getFileContentType(".html"),-1);
        sendFile("404.html",cfd);
        return 0;
    }
    

    if(S_ISDIR(st.st_mode))
    {
        //send the files in the folder
        sendHeadMsg(cfd, 200,"OK", getFileContentType(".html"),-1); 
        sendDir(file, cfd);
    }
    else if(strstr(file,"favicon.ico"))//to void web request ico
    {
        sendHeadMsg(cfd,204,"No Content",getFileContentType(".txt"),-1);
    }
    else
    {
        //send file to client
        sendHeadMsg(cfd,200,"OK",getFileContentType(file),st.st_size);
        sendFile(file,cfd);
    }
    
    return 0;
}


int sendFile(const char* filename ,int cfd)
{
    //openfile
    int fd = open(filename,O_RDONLY);
    assert(fd > 0);    

//send file by custom method
#if 0    
    while(1)
    {
        char buf[1024];
        int len = read(fd,buf,sizeof(buf));
        if(len > 0)
        {
            send(cfd,buf,len,0);
            usleep(10); //important!!! to reduce recver pressure.
        }
        else if(len == 0)
        {
            break;
        }
        else 
        {
            perror("read");
            return -1;
        }
    }
#else
//send file by the method Linux provided
    off_t offset = 0;
    int size = lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET); //reset file pointer
    while(offset < size)
    {
        int res = sendfile(cfd, fd, &offset, size - offset);   
        if(res == -1 && errno != EAGAIN)//如果是阻塞模式就不用担心fd的打开和cfd的上传节奏跟不上的问题
        {
            perror("sendfile");
            close(fd);
            return -1;
        }
    }
#endif
    close(fd);
    return 0;
}

int sendHeadMsg(int cfd, int status, const char *descr, const char *type, int length)
{
    //status line
    char buf[4096] = {0};
    sprintf(buf, "http/1.1 %d %s\r\n", status,descr);
    
    //response head
    sprintf(buf + strlen(buf), "content-type: %s\r\n",type);
    sprintf(buf+strlen(buf), "content-length: %d\r\n",length);
    sprintf(buf+strlen(buf),"\r\n");

    send (cfd,buf,strlen(buf), 0);

    return 0;
}


const char* getFileContentType(const char* filename)
{
    // find point from right
    const char* dot = strrchr(filename, '.');
    if(dot == NULL)
        return "text/plain; cherset=utf-8";
    if(strcmp(dot,".html") == 0 || strcmp(dot,".htm") == 0)
        return "text/html; charset=utf-8";
    if(strcmp(dot,".jpg") == 0 || strcmp(dot,".jpeg") == 0)
        return "image/jpeg";
    if(strcmp(dot,".gif") == 0)
        return "image/gif";
    if(strcmp(dot,".png") == 0)
        return "image/png";
    if(strcmp(dot,".css") == 0)
        return "text/css";
    if(strcmp(dot,".au") == 0)
        return "audio/basic";
    if(strcmp(dot,".wav") == 0)
        return "audio/wav";
    if(strcmp(dot,".avi") == 0)
        return "video/avi";
    if(strcmp(dot,".mpeg") == 0)
        return "video/mpg";
    if(strcmp(dot,".mp3") == 0)
        return "audio/mp3";
    if(strcmp(dot,".pic") == 0)
        return "application/x-pic";
    if(strcmp(dot,".gif") == 0)
        return "image/gif";
    return "text/plain; cherset=utf-8";
}

/* a simple html frame
 <html>
    <head>
        <title>tiele</title>
    </head>
    <body>
        <table>
            <tr>
                <td></td>
                <td></td>
            </tr>
            <tr>
                <td></td>
                <td></td>
            </tr>
        </table>
    </body>
 </html>
 */

int sendDir(const char* dirname, int cfd)
{
    char buff[4096] = { 0 };
    sprintf(buff,"<html><head><title>%s</title></head><body><table>",dirname); 
    struct dirent **namelist;
    int dirnum = scandir(dirname,&namelist, NULL, alphasort);
    for (int i = 0; i < dirnum; i++)
    {
        //get filename
        
        char* name = namelist[i]->d_name;
        struct stat st;
        char subPath[1024] = {0};
        sprintf(subPath,"%s/%s",dirname,name);
        stat(subPath,&st);
        if(S_ISDIR(st.st_mode))
        {
            sprintf(buff+strlen(buff),"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
                    name,name,st.st_size);
        }
        else 
        {
            sprintf(buff+strlen(buff),"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
                    name,name,st.st_size);   
        }
        send(cfd,buff,strlen(buff),0);
        memset(buff,0,sizeof(buff));
        free(namelist[i]);
    }
    sprintf(buff,"</table></body></html>");
    send(cfd,buff,strlen(buff),0);
    free(namelist);
    return 0;   
}


int hexToDec(char c)
{
    if(c >= '0' && c <= '9')
        return c-'0';
    if(c >='a' && c <= 'f')
        return c - 'a'+10;
    if(c >= 'A' && c<= 'F')
        return c - 'A'+10;
    return 0;
}


void decodeMsg(char* to,char* from)
{
    for(;*from != '\0';to++ ,from++)
    {
        if(from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            *to = hexToDec(from[1])*16 + hexToDec(from[2]);  
            from +=2;
        }
        else
        {
            *to = *from;
        }
    }

}
