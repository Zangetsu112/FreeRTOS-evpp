#ifndef EXAMPLE_TCP_ECHO_H
#define EXAMPLE_TCP_ECHO_H

#include "EventLoop.h"
#include "tcp_server.h"
#include "evpp_buffer.h"
#include "FreeRTOS.h"
#include "task.h"

class TestTask {
public:
    static void taskWrapper(void* pvParameters);
    static void run();
    static void OnMessage(const TCPConnPtr& conn, Buffer* msg);
    static void OnConnection(const TCPConnPtr& conn);
};

extern "C" void start_tcp_task(void);

// extern "C" void start_tcp_task();
// extern "C" void taskWrapper_wrapper(void *pvParameters);

// class TestTask {
// public:
//     static void taskWrapper(void* pvParameters);
//     void run();

// public:
//     static void OnMessage(const TCPConnPtr& conn, Buffer* msg);
//     static void OnConnection(const TCPConnPtr& conn);
//     static void start();
// };

#endif // EXAMPLE_TCP_ECHO_H
