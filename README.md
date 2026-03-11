# taskBasedTcpServer

基于 **Reactor 模式**实现的 C++17 高性能 TCP 服务器框架。采用 **epoll 单线程 I/O 多路复用 + 线程池异步任务处理**架构，通过抽象 `Task` 基类和 `TaskFactory` 工厂函数将网络框架与业务逻辑彻底解耦，适合计算密集型场景下的并发服务端开发。

## 核心设计目标

- **I/O 与计算分离**：主线程（EventLoop）只负责 epoll 事件分发，业务计算全部投递给线程池
- **线程安全的结果回传**：worker 线程处理完任务后，通过 `eventfd` 机制安全地将发送操作交还主线程执行
- **业务与框架解耦**：通过 `Task` 抽象基类 + `TaskFactory` 工厂函数，框架层无需感知业务细节

---

## 架构概览

```
┌─────────────────────────────────────────────────────────────────┐
│                        TaskBasedTcpServer                        │
│                 （高层封装，整合框架与业务）                       │
│         ThreadPool ──────────── TcpServer                        │
│              │                     │                             │
│         TaskFactory            Acceptor + EventLoop              │
└─────────────────────────────────────────────────────────────────┘
         │                               │
┌────────┴────────┐             ┌────────┴────────┐
│   business 层   │             │   reactor 层     │
│  MyTask         │             │  EventLoop       │
│  （示例业务）    │             │  TcpConnection   │
└────────-────────┘             │  ThreadPool      │
                                │  TaskQueue       │
                                │  Socket/SocketIO │
                                └─────────────────┘
```

### 完整数据流

```
客户端发送数据
    │
    ▼
epoll_wait 检测到 connfd 可读
    │
    ▼
EventLoop::handleMessage()
    │
    ▼
TcpConnection::handleMessageCallback()
    │
    ▼
TaskBasedTcpServer::onMessage()
    ├─ conn->recive()          ← 从 socket 读取数据（主线程）
    ├─ taskFactory(conn, msg)  ← 创建具体 Task 对象
    └─ pool.addTask(task)      ← 投入线程池队列
                                    │
                                    ▼
                            worker 线程执行 Task::process()
                                    │
                                    ▼
                            conn->sendInLoop(result)
                                    │
                            EventLoop::runInLoop(functor)
                                    ├─ 将 functor 加入 m_pendings
                                    └─ 写 eventfd 唤醒 epoll
                                            │
                                            ▼
                                    epoll 检测到 evtfd 可读
                                            │
                                    doPengdingFunctors()
                                            │
                                            ▼
                                    主线程执行 send()  ← 结果发回客户端
```

---

## 项目结构

```
taskBasedTcpServer/
├── main.cpp                          # 入口，演示多种启动方式
├── CMakeLists.txt
├── reactor/                          # 网络框架层（通用，不含业务）
│   ├── include/
│   │   ├── TaskBasedTcpServer.h      # 高层封装：TcpServer + ThreadPool + TaskFactory
│   │   ├── TcpServer.h               # TCP 服务器：持有 Acceptor + EventLoop
│   │   ├── EventLoop.h               # epoll 事件循环核心
│   │   ├── Acceptor.h                # 监听 socket 封装
│   │   ├── TcpConnection.h           # 单连接封装，三回调 + sendInLoop
│   │   ├── ThreadPool.h              # 固定大小线程池
│   │   ├── TaskQueue.h               # 有界阻塞任务队列
│   │   ├── Task.h                    # 任务抽象基类
│   │   ├── Socket.h                  # socket fd RAII 封装
│   │   ├── SocketIO.h                # readn / writen / readline
│   │   ├── InetAddress.h             # sockaddr_in 封装
│   │   └── utils/NonCopyable.h       # 禁止拷贝的基类
│   └── src/                          # 对应实现文件
└── business/                         # 业务逻辑层（示例）
    ├── include/MyTask.h              # Task 的具体实现示例
    └── src/MyTask.cpp
```

---

## 技术详解

### 1. EventLoop — epoll 事件循环

`EventLoop` 是整个框架的核心，运行在**主线程**，负责所有 I/O 事件的检测与分发。

```
EventLoop 监听三类 fd：
  ┌─ listenfd（Acceptor）  → handleNewConnection()
  ├─ evtfd（eventfd）      → handleEventFdRead() + doPengdingFunctors()
  └─ connfd（各客户端）    → handleMessage(fd)
```

**关键实现细节：**

- **epoll 事件列表自动扩容**：每次 `epoll_wait` 返回时，若就绪事件数等于列表容量，则将 `m_evtList` 扩容为两倍，防止事件丢失

  ```cpp
  if (nready == static_cast<int>(m_evtList.size())) {
      m_evtList.resize(m_evtList.size() * 2);
  }
  ```

- **连接管理**：所有活跃连接存储在 `std::map<int, shared_ptr<TcpConnection>> m_conns` 中，以 fd 为键；连接关闭时从 epoll 红黑树删除 fd 并从 map 中移除

- **EINTR 处理**：`epoll_wait` 被信号中断（返回 -1 且 `errno == EINTR`）时循环重试，避免意外退出

### 2. eventfd 跨线程唤醒机制

这是框架实现**线程安全结果回传**的核心机制，解决了"worker 线程无法直接调用 send"的问题。

**问题**：worker 线程处理完业务后需要向客户端发送数据，但 socket 的 send 操作应在 EventLoop 主线程中执行，否则存在竞态。

**解决方案**：

```
worker 线程
    └─ conn->sendInLoop(msg)
            └─ loop->runInLoop([conn, msg]{ conn->send(msg); })
                    ├─ lock(m_mutex)
                    ├─ m_pendings.push_back(functor)  ← 存入待办队列
                    └─ write(evtfd, 1)                ← 唤醒 epoll

主线程（epoll 被唤醒）
    └─ handleEventFdRead()   ← read(evtfd) 清空计数器
    └─ doPengdingFunctors()
            ├─ lock(m_mutex)
            ├─ swap(m_pendings, tmp)  ← swap trick：最小化锁持有时间
            └─ 遍历 tmp，执行每个 functor（即 send）
```

**swap trick**：`doPengdingFunctors` 中先将 `m_pendings` swap 到局部变量 `tmp`，再释放锁，最后遍历执行。这样执行 functor 期间不持有锁，避免新任务投递被阻塞。

### 3. TaskQueue — 有界阻塞队列（生产者-消费者）

`TaskQueue` 是线程池的任务缓冲区，基于**双条件变量**实现有界阻塞队列：

```
生产者（主线程 addTask）         消费者（worker 线程 getTask）
        │                                   │
   m_notFull.wait()                   m_notEmpty.wait()
   （队列满时阻塞）                    （队列空时阻塞）
        │                                   │
   push → m_notEmpty.notify_one()    pop → m_notFull.notify_one()
```

| 条件变量 | 含义 | 生产者行为 | 消费者行为 |
|----------|------|-----------|-----------|
| `m_notFull` | 队列未满 | 满时 wait，pop 后 notify | push 后 notify |
| `m_notEmpty` | 队列非空 | push 后 notify | 空时 wait，push 后 notify |

**优雅退出**：`wakeAll()` 将 `m_isRunning` 置为 false 并 `notify_all()`，阻塞在 `m_notEmpty.wait()` 的 worker 线程全部唤醒，`pop()` 返回 `nullptr`，worker 线程检测到后退出循环。

### 4. ThreadPool — 固定大小线程池

```
ThreadPool::start()
    └─ 创建 N 个 std::thread，每个执行 doTask()

doTask() 工作循环：
    while (!m_isExit) {
        TaskFunc task = getTask();  ← 阻塞等待任务
        if (task) task();           ← 执行任务（多态调用）
    }

ThreadPool::stop() 优雅退出：
    1. 忙等待直到队列清空（usleep 轮询）
    2. m_isExit = true
    3. taskQueue.wakeAll()  ← 唤醒所有阻塞的 worker
    4. 逐一 join()
```

任务以 `std::function<void()>`（即 `TaskFunc`）存储，调用时触发 `Task::process()` 的多态分发。

### 5. TcpConnection — 连接封装与生命周期管理

`TcpConnection` 继承 `std::enable_shared_from_this`，使其能在回调中安全持有自身的 `shared_ptr`，防止在异步执行过程中对象被提前析构。

**三回调模型**：

| 回调 | 触发时机 | 默认行为 |
|------|---------|---------|
| `onConnection` | 三次握手完成，连接建立 | 发送欢迎消息 |
| `onMessage` | connfd 可读，有数据到达 | 读取数据，创建 Task 投入线程池 |
| `onClose` | 对端关闭连接（read 返回 0） | 从 epoll 和 map 中清理连接 |

**`sendInLoop()`**：worker 线程调用此方法，通过 `EventLoop::runInLoop()` 将实际的 `send()` 调度到主线程执行，保证线程安全。

### 6. Acceptor — 监听 socket 封装

`Acceptor::ready()` 在构建时完成以下初始化：
1. `SO_REUSEADDR`：允许地址复用，服务重启时无需等待 TIME_WAIT
2. `SO_REUSEPORT`：允许端口复用，支持多进程/线程监听同一端口
3. `bind()`：绑定 IP 和端口
4. `listen()`：进入监听状态

### 7. TaskBasedTcpServer — 框架与业务的桥梁

`TaskBasedTcpServer` 是对 `TcpServer + ThreadPool` 的高层封装，引入 **TaskFactory** 工厂函数模式：

```cpp
// 使用方只需提供一个工厂函数，框架自动处理 I/O 和调度
server.setTaskFactory([](shared_ptr<TcpConnection> conn, string msg) {
    return make_shared<MyTask>(move(conn), move(msg));
});
server.start();
```

这样框架层（reactor）完全不依赖具体业务类，业务层只需继承 `Task` 并实现 `process()`。

### 8. Socket / SocketIO — RAII 资源管理

- **`Socket`**：持有 socket fd，析构时自动 `close(fd)`，防止 fd 泄漏
- **`SocketIO`**：提供三个可靠 IO 操作：
  - `readn(buf, n)` — 循环读直到读满 n 字节（处理短读）
  - `writen(buf, n)` — 循环写直到写完 n 字节（处理短写）
  - `readline(buf, max)` — 逐字节读取直到遇到 `\n`

---

## 启动方式演进（main.cpp）

`main.cpp` 中保留了框架演进的三个版本：

| 版本 | 函数 | 说明 |
|------|------|------|
| V1 | `testReactor()` | 基础版：TcpServer + 全局 ThreadPool，手动绑定回调 |
| V4 | `testReactorV4()` | 同 V1，显式调用 `pool.start()` |
| **V4.1** | `testReactorV41()` | **当前默认**：TaskBasedTcpServer + TaskFactory，最简洁 |

当前 `main()` 调用 `testReactorV41()`，是最终封装形态。

---

## Task 扩展方式

继承 `Task` 实现 `process()` 即可接入框架：

```cpp
// MyTask 示例（business/src/MyTask.cpp）
void MyTask::process() {
    // 1. 处理业务（此处模拟计算耗时）
    ::sleep(1);
    m_sendMsg = "from Server:" + m_recvMsg;

    // 2. 将结果安全回传到 EventLoop 主线程发送
    m_conn->sendInLoop(m_sendMsg);
}
```

实际业务中可在 `process()` 中执行数据库查询、文件 IO、复杂计算等任意耗时操作，框架保证其在 worker 线程中执行，不阻塞主线程的 I/O 处理。

---

## 环境要求

- **OS**：Linux（依赖 `epoll` 和 `eventfd`，不支持 Windows/macOS）
- **编译器**：GCC / Clang，需支持 C++17
- **CMake**：>= 3.20

## 构建与运行

```bash
mkdir build && cd build
cmake ..
cmake --build .
./edoyun_access2_server
```

默认监听 `127.0.0.1:8888`，线程池 4 个 worker，任务队列容量 10。

修改地址或并发参数：编辑 `main.cpp` 中 `TaskBasedTcpServer` 的构造参数：

```cpp
TaskBasedTcpServer server(/*threadNum=*/4, /*queSize=*/10, "127.0.0.1", 8888);
```
