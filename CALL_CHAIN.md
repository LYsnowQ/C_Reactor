# Reactor 调用链说明

本文基于当前代码（`EventLoop.c` 默认使用 `PollDispatcher`）梳理：
- 首次启动调用链
- 单次 HTTP 请求调用链
- `Epoll / Poll / Select` 在事件分发处的差异

## 1. 首次启动调用链

### 1.1 进程入口与服务器初始化

```text
main()
  -> chdir(argv[2])
  -> tcpServerInit(port, 4)
      -> listenerInit(port)
          -> socket()
          -> setsockopt(SO_REUSEADDR)
          -> bind()
          -> listen()
      -> eventLoopInit()
          -> eventLoopInitEx(NULL)
              -> dispatcher = &PollDispatcher
              -> dispatcher->init()
              -> channelMapInit()
              -> socketpair()
              -> channelInit(socketPair[1], ReadEvent, readLocalMessage, ...)
              -> eventLoopAddTask(..., ADD)   // 注册唤醒通道
      -> threadPoolInit(mainLoop, threadNum)
```

### 1.2 启动线程池与主事件循环

```text
tcpServerRun(server)
  -> threadPoolRun(threadPool)
      -> for each worker:
          -> workerThreadInit()
          -> workerThreadRun()
              -> pthread_create(subThreadRunning)
              -> 等待子线程把 evLoop 初始化完成
  -> channelInit(listener->lfd, ReadEvent, acceptConnection, ...)
  -> eventLoopAddTask(mainLoop, listenerChannel, ADD)
  -> eventLoopRun(mainLoop)
      -> while (!isQuit):
          -> dispatcher->dispatch(evLoop, 2)
          -> eventLoopProcessTask(evLoop)
```

### 1.3 工作线程启动链

```text
subThreadRunning()
  -> eventLoopInitEx(threadName)
      -> dispatcher = &PollDispatcher
      -> dispatcher->init()
      -> socketpair()
      -> 注册本线程 wakeup channel
  -> eventLoopRun(workerLoop)   // 子线程进入自己的事件循环
```

## 2. 单次 HTTP 请求调用链

> 以浏览器请求 `/` 为例。

### 2.1 建连阶段（主线程）

```text
mainLoop dispatcher 检测到 listener lfd 可读
  -> eventActivate(mainLoop, lfd, ReadEvent)
      -> acceptConnection(server)
          -> accept(listener->lfd) 得到 cfd
          -> takeWorkerEventLoop(threadPool)   // 轮询选择 worker loop
          -> tcpConnectionInit(cfd, workerLoop)
              -> bufferInit(read/write)
              -> httpRequestInit()
              -> httpResponseInit()
              -> channelInit(cfd, ReadEvent, processRead, processWrite, ...)
              -> eventLoopAddTask(workerLoop, connChannel, ADD)
```

### 2.2 读请求与解析（工作线程）

```text
workerLoop dispatcher 检测到 cfd 可读
  -> eventActivate(workerLoop, cfd, ReadEvent)
      -> processRead(conn)
          -> bufferSocketRead(readBuffer, cfd)
          -> parseHttpRequest(request, readBuffer, response, writeBuffer, cfd)
              -> parseHttpRequestLine()    // 解析请求行
              -> parseHttpRequestHead()    // 逐行解析请求头
              -> processHttpRequest()      // 路由到文件/目录/404
              -> httpResponsePrepareMsg()  // 组装状态行+响应头并发送
                  -> sendFile() 或 sendDir()
```

### 2.3 关闭连接（当前默认非 keep-alive）

```text
processRead() 结束
  -> eventLoopAddTask(workerLoop, connChannel, DELETE)
  -> eventLoopProcessTask()
      -> eventLoopRemove()
          -> dispatcher->remove()
              -> tcpConnectionDestory()
                  -> destoryChannel()
                  -> bufferDestory()
                  -> httpRequestDestory()
                  -> httpResponseDestory()
```

## 3. 分发器差异（关键只在 dispatch 层）

三种模型共享同一套上层逻辑：
- `eventLoopAddTask / eventLoopProcessTask / eventActivate`
- `acceptConnection / tcpConnectionInit / processRead / processWrite`

差异点只有 `dispatcher->add/remove/modify/dispatch` 的实现：

### 3.1 Epoll

```text
dispatch -> epoll_wait()
  -> 遍历 ready events
  -> eventActivate(evLoop, ready_fd, ReadEvent/WriteEvent)
```

### 3.2 Poll

```text
dispatch -> poll()
  -> 遍历 pollfd[]
  -> eventActivate(evLoop, pollfd.fd, ReadEvent/WriteEvent)
```

### 3.3 Select

```text
dispatch -> select()
  -> 用 readSet/writeSet 拷贝到临时 fd_set
  -> FD_ISSET(i) 后调用 eventActivate(evLoop, i, ...)
```

## 4. 一句话总览

```text
启动: main -> tcpServerInit -> threadPoolRun + mainLoopRun
请求: listener可读 -> acceptConnection -> tcpConnectionInit -> processRead -> parseHttpRequest -> httpResponsePrepareMsg -> (sendFile/sendDir) -> remove/destroy
```
