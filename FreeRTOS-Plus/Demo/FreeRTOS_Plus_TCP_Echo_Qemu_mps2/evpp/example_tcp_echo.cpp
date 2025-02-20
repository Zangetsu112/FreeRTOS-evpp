#include "example_tcp_echo.h"
#include "FreeRTOSConfig.h"
#include "tcp_conn.h"
#include "task.h"


extern "C" {
    void start_tcp_task() {
        xTaskCreate(TestTask::taskWrapper, "TestEVPPTask", configMINIMAL_STACK_SIZE * 2, NULL, 8, NULL);
    }
}

void TestTask::taskWrapper(void* pvParameters) {
    (void)pvParameters;
    TestTask::run();
}

void TestTask::run() {
    printf("Hello from evvp task\n");
    std::string port = "9999";
    std::string addr = std::string("0.0.0.0:") + port;
    EventLoop loop;
    TCPServer server(&loop, addr, "TCPEcho", 2);
    server.SetMessageCallback(&OnMessage);
    server.SetConnectionCallback(&OnConnection);
    server.Init();
    server.Start();
    printf("Server stated\n");
    loop.Run();
    printf("This shouldn't print\n");
}

void TestTask::OnMessage(const TCPConnPtr& conn, Buffer* msg) {
    std::string s = msg->NextAllString();
    std::string str;
    if (s.back() == '\n')
        str = s.substr(0, s.size() - 1);
    if (str.back() == '\r')
        str = str.substr(0, str.size() - 1);
    printf("Received message: [ %s ]\n", str.c_str());
    conn->Send(s);

    if (str == "quit" || str == "exit") {
        conn->Close();
    }
}

void TestTask::OnConnection(const TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        printf("Accept a new connection from %s\n", conn->remote_addr().c_str());
    } else {
        printf("Disconnected from %s\n", conn->remote_addr().c_str());
    }
}

