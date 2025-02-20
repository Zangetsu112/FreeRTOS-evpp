#pragma once

#include <string>
#include "fd_channel.h"
#include "FreeRTOS_Sockets.h"

class EventLoop;
class FdChannel;

class Listener {
public:
    typedef std::function <
    void(Socket_t sockfd,
         const std::string& /*remote address with format "ip:port"*/,
         const struct freertos_sockaddr* /*remote address*/) >
    NewConnectionCallback;
    Listener(EventLoop* loop, const std::string& addr/*local listening address : ip:port*/);
    ~Listener();

    // socket listen
    void Listen(int backlog = 20);

    // nonblocking accept
    void Accept();

    void Stop();

    void SetNewConnectionCallback(NewConnectionCallback cb) {
        new_conn_fn_ = cb;
    }

private:
    void HandleAccept();

private:
    Socket_t fd_ = FREERTOS_INVALID_SOCKET;// The listening socket fd
    EventLoop* loop_;
    std::string addr_;
    std::unique_ptr<FdChannel> chan_;
    NewConnectionCallback new_conn_fn_;
};



