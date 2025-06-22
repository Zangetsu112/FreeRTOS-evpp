#include "example_tcp_echo.h"
#include "FreeRTOSConfig.h"
#include "tcp_conn.h"
#include "task.h"


extern "C" {
    void start_tcp_task() {
        xTaskCreate(TestTask::taskWrapper, "TestEVPPTask", configMINIMAL_STACK_SIZE , NULL, 2, NULL);
    }
}

void TestTask::taskWrapper(void* pvParameters) {
    (void)pvParameters;
    TestTask::run();
}

void TestTask::run() {
    FreeRTOS_debug_printf(("Hello from evvp task\n"));
    std::string port = "9999";
    std::string addr = std::string("0.0.0.0:") + port;
    EventLoop loop;
    TCPServer server(&loop, addr, "TCPEcho", 7);
    server.SetMessageCallback(&OnMessage);
    server.SetConnectionCallback(&OnConnection);
    server.Init();
    server.Start();
    FreeRTOS_debug_printf(("Server started\n"));
    loop.Run();
    FreeRTOS_debug_printf(("This shouldn't print\n"));
}

void TestTask::OnMessage(const TCPConnPtr& conn, Buffer* msg) {
    // TickType_t start_time = xTaskGetTickCount();
    std::string s = msg->NextAllString();
    std::string str;
    if (s.back() == '\n')
        str = s.substr(0, s.size() - 1);
    if (str.back() == '\r')
        str = str.substr(0, str.size() - 1);
    // FreeRTOS_debug_printf(("Received message: [ %s ]\n", str.c_str()));
    conn->Send("(Server Response) " + s);

    if (str == "quit" || str == "exit") {
        conn->Close();
    }
    // TickType_t start_time = xTaskGetTickCount();
}

void TestTask::OnConnection(const TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        FreeRTOS_debug_printf(("Accepted a new connection from %s\n", conn->remote_addr().c_str()));
    } else {
        FreeRTOS_debug_printf(("Disconnected from %s\n", conn->remote_addr().c_str()));
    }
}

