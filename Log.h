#pragma once

#include <stdio.h>
#include <time.h>

 
#define _LOG_DEBUG      // 全开所有日志（DEBUG+INFO+WARN+ERROR）
//#define _LOG_INFO     // 除DEBUG外全开（INFO+WARN+ERROR）
//#define _LOG_WARN     // 仅WARN+ERROR
//#define _LOG_ERROR    // 仅ERROR
// 不定义任何宏 = 关闭所有日志（Release模式）


#if defined(_LOG_DEBUG)
#define LOG_LEVEL 0
#elif defined(_LOG_INFO)
#define LOG_LEVEL 1
#elif defined(_LOG_WARN)
#define LOG_LEVEL 2
#elif defined(_LOG_ERROR)
#define LOG_LEVEL 3
#else
#define LOG_LEVEL 4   /* 默认关闭所有日志 */
#endif

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3

#define _LOG_PRINT(level,tag,fmt,...) do{\
    time_t _t = time(NULL);\
    struct tm* _tm = localtime(&_t);\
    char _b[20];\
    strftime(_b,20,"%Y-%m-%d %H:%M:%S",_tm);\
    fprintf(stderr,"[%s][%s][%s:%d][%s] "fmt"\n",\
        _b,tag,__FILE__,__LINE__,__func__,##__VA_ARGS__);\
}while(0)

#define LOG_DEBUG(fmt,...) do{if(LOG_LEVEL<=LOG_LEVEL_DEBUG)_LOG_PRINT(0,"DEBUG",fmt,##__VA_ARGS__);}while(0)
#define LOG_INFO(fmt,...)  do{if(LOG_LEVEL<=LOG_LEVEL_INFO)_LOG_PRINT(1,"INFO",fmt,##__VA_ARGS__);}while(0)
#define LOG_WARN(fmt,...)  do{if(LOG_LEVEL<=LOG_LEVEL_WARN)_LOG_PRINT(2,"WARN",fmt,##__VA_ARGS__);}while(0)
#define LOG_ERROR(fmt,...) do{if(LOG_LEVEL<=LOG_LEVEL_ERROR)_LOG_PRINT(3,"ERROR",fmt,##__VA_ARGS__);}while(0)

