#include <unistd.h>
#include <iostream>
#include <memory>
#include "TaskBasedTcpServer.h"
#include "MyTask.h"
#include "TcpConnection.h"
#include "TcpServer.h"
#include "ThreadPool.h"

ThreadPool pool(4, 10);

// connection established
void onNewConnection(std::shared_ptr<TcpConnection> const& _conn)
{
    _conn->send("Welcome to server!\n");
}

// connection readable
void onMessage(std::shared_ptr<TcpConnection> const& _conn)
{
    // receive the data from the client.
    std::string msg = _conn->recive();

    // Process the business logic here.

    std::cout << "onMessage: received message from " << _conn->toString() << ": " << msg;

    // The business processing is completed. The result data will be sent to the client.
    MyTask myTask(_conn, msg);
    pool.addTask(std::bind(&MyTask::process, myTask));
}

// connection closed
void onClose(std::shared_ptr<TcpConnection> const& _conn)
{
    std::cout << "onClose: " << _conn->toString() << " has closed." << std::endl;
}

int testReactor(int argc, char* argv[])
{
    TcpServer server("127.0.0.1", 8888);
    server.setAllCallbacks(std::move(onNewConnection), std::move(onMessage), std::move(onClose));
    server.start();

    return 0;
}

// int testThreadPool(int argc, char* argv[])
// {
//     ThreadPool threadPool(4, 10);
//     // create and start the thread pool
//     threadPool.start();

//     // simulate adding tasks to the thread pool
//     for (int i = 0; i < 20; ++i)
//     {
//         auto pTask{new MyTask()};
//         threadPool.addTask(pTask);
//     }

//     // close the thread pool
//     threadPool.stop();
//     return 0;
// }

void testReactorV4()
{
    pool.start();

    TcpServer server("127.0.0.1", 8888);
    server.setAllCallbacks(std::move(onNewConnection), std::move(onMessage), std::move(onClose));
    server.start();
}

void testReactorV41()
{
    TaskBasedTcpServer server(4, 10, "127.0.0.1", 8888);
    server.setTaskFactory([](std::shared_ptr<TcpConnection> conn, std::string msg) {
        return std::make_shared<MyTask>(std::move(conn), std::move(msg));
    });
    server.start();
}

int main(int argc, char* argv[])
{
    // testThreadPool(argc, argv);
    testReactorV41();
    return 0;
}