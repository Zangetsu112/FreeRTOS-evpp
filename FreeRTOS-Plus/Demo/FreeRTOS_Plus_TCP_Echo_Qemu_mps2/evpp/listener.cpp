#include "inner_pre.h"
#include "listener.h"
#include "EventLoop.h"
#include "FreeRTOS_Sockets.h"

Listener::Listener(EventLoop* l, const std::string& addr)
    : loop_(l), addr_(addr) {}

Listener::~Listener() {
    chan_.reset();
    FreeRTOS_closesocket(fd_);
    fd_ = FREERTOS_INVALID_SOCKET;
}

void Listener::Listen(int backlog) {
    TickType_t xNoTimeOut = 0; // For nonblocking sockets

    // Parse IP and port from the address string
    char ip[16];
    uint16_t port;
    sscanf(addr_.data(), "%15[^:]:%hu", ip, &port);

    // Create socket
    fd_ = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    if (fd_ == FREERTOS_INVALID_SOCKET) {
        FreeRTOS_debug_printf(("Failed to create socket\n"));
        return;
    }

    // Set socket to non-blocking mode
    BaseType_t xResult = FreeRTOS_setsockopt(fd_, 0, FREERTOS_SO_RCVTIMEO, &xNoTimeOut, sizeof(xNoTimeOut));
    if (xResult != 0) {
        FreeRTOS_debug_printf(("Failed to bind socket\n"));
        FreeRTOS_closesocket(fd_);
        fd_ = FREERTOS_INVALID_SOCKET;
        return;
    }

    // Bind socket
    struct freertos_sockaddr bind_addr;
    bind_addr.sin_port = FreeRTOS_htons(port);
    bind_addr.sin_address.ulIP_IPv4 =  FreeRTOS_inet_addr(ip);
    bind_addr.sin_family = FREERTOS_AF_INET4;

    BaseType_t bind_result = FreeRTOS_bind(fd_, &bind_addr, sizeof(bind_addr));
    if (bind_result != 0) {
        FreeRTOS_debug_printf(("Bind Error. Addr=%s\n", addr_));
        FreeRTOS_closesocket(fd_);
        fd_ = FREERTOS_INVALID_SOCKET;
        return;
    }

    // Start listening
    BaseType_t listen_result = FreeRTOS_listen(fd_, backlog);
    if (listen_result != 0) {
        FreeRTOS_debug_printf(("Socket Listen Failed\n"));
        FreeRTOS_closesocket(fd_);
        fd_ = FREERTOS_INVALID_SOCKET;
        return;
    }
}

void Listener::Accept() {
    chan_.reset(new FdChannel(loop_, fd_, true, false));
    // chan_->SetReadCallback(std::bind(&Listener::HandleAccept, this));
    // loop_->RunInLoop(std::bind(&FdChannel::AttachToLoop, chan_.get()));

    chan_->SetReadCallback([this]() { this -> HandleAccept(); });
    loop_->RunInLoop([this]() { this -> chan_ -> AttachToLoop(); }); 
}

void Listener::HandleAccept() {
    FreeRTOS_debug_printf(("Incoming connection\n"));
    configASSERT(loop_->IsInLoopThread());

    struct freertos_sockaddr client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Accept the new connection
    Socket_t new_socket = FreeRTOS_accept(fd_, &client_addr, &addr_len);

    // Set the new socket to non-blocking mode
    TickType_t xNoTimeout = 0;
    BaseType_t xResult = FreeRTOS_setsockopt(new_socket, 0, FREERTOS_SO_RCVTIMEO, &xNoTimeout, sizeof(xNoTimeout));
    if (xResult != 0) {
        // LOG_ERROR("Set socket non-blocking failed");
        FreeRTOS_debug_printf(("Set slave socket non-blocking failed\n"));
        FreeRTOS_closesocket(new_socket);
        return;
    }

    // Set keep-alive: HOW???
    // For now, have the Keep alive config set in the IP Config file

    // Convert IP address to string
    char raddr[32];
    uint32_t ip_address = client_addr.sin_address.ulIP_IPv4; // Correct member is sin_address.ulIP_IPv4
    FreeRTOS_inet_ntoa(ip_address, raddr);
    uint16_t port = FreeRTOS_ntohs(client_addr.sin_port);
    snprintf(raddr + strlen(raddr), sizeof(raddr) - strlen(raddr), ":%u", port);

    if (strlen(raddr) == 0) {
        FreeRTOS_debug_printf(("Failed to get remote address\n"));
        FreeRTOS_closesocket(new_socket);
        return;
    }

    FreeRTOS_debug_printf(( "Accepted a connection from %s, listen socket=%p, client socket=%p\n",
                            raddr, pvSocketGetSocketID(fd_), pvSocketGetSocketID(new_socket)));

    if (new_conn_fn_) {
       new_conn_fn_(new_socket, raddr, &client_addr);
    }
}

void Listener::Stop() {
    configASSERT(loop_->IsInLoopThread());
    chan_->DisableAllEvent();
    chan_->Close();
}
