#pragma once

#include "evpp_buffer.h"
#include "FreeRTOS_Sockets.h"
#include "EventLoop.h"
#include "tcp_callbacks.h"
#include "slice.h"
#include "atomic.h"
#include "fd_channel.h"

class EventLoop;
// class TCPClient;

class TCPConn : public std::enable_shared_from_this<TCPConn> {
public:
    enum Type {
        kIncoming = 0, // The type of a TCPConn held by a TCPServer
        kOutgoing = 1, // The type of a TCPConn held by a TCPClient
    };
    enum Status {
        kDisconnected = 0,
        kConnecting = 1,
        kConnected = 2,
        kDisconnecting = 3,
    };
public:
    TCPConn(EventLoop* loop,
            const std::string& name,
            Socket_t sockfd,
            const std::string& laddr,
            const std::string& raddr,
            uint64_t id);
    ~TCPConn();

    void Close();

    void Send(const char* s) {
        Send(s, strlen(s));
    }
    void Send(const void* d, size_t dlen);
    void Send(const std::string& d);
    void Send(const Slice& message);
    void Send(Buffer* buf);
public:
    EventLoop* loop() const {
        return loop_;
    }
    Socket_t fd() const {
        return fd_;
    }
    uint64_t id() const {
        return id_;
    }
    // Return the remote peer's address with form "ip:port"
    const std::string& remote_addr() const {
        return remote_addr_;
    }
    const std::string& name() const {
        return name_;
    }
    bool IsConnected() const {
        return status_ == kConnected;
    }
    bool IsConnecting() const {
        return status_ == kConnecting;
    }
    bool IsDisconnected() const {
        return status_ == kDisconnected;
    }
    bool IsDisconnecting() const {
        return status_ == kDisconnecting;
    }
    Type type() const {
        return type_;
    }
    bool IsIncommingConn() const {
        return type_ == kIncoming;
    }
    Status status() const {
        return status_;
    }

    std::string AddrToString() const {
        if (IsIncommingConn()) {
            return "(" + remote_addr_ + "->" + local_addr_ + "(local))";
        } else {
            return "(" + local_addr_ + "(local)->" + remote_addr_ + ")";
        }
    }

    // @brief When it is an incoming connection, we need to preserve the
    //  connection for a while so that we can reply to it. And we set a timer
    //  to close the connection eventually.
    // @param[in] d - If d.IsZero() == true, we will close the connection immediately.
    // void SetCloseDelayTime(Duration d) {
    //     assert(type_ == kIncoming);
    //     // This option is only available for the connection type kIncoming
    //     // Set the delay time to close the socket
    //     close_delay_ = d;
    // }

public:
    // void SetTCPNoDelay(bool on);

    // TODO Add : SetLinger();

    // void ReserveInputBuffer(size_t len) { input_buffer_.Reserve(len); }
    // void ReserveOutputBuffer(size_t len) { output_buffer_.Reserve(len); }

    void SetWriteCompleteCallback(const WriteCompleteCallback cb) {
        write_complete_fn_ = cb;
    }

    void SetHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t mark);
    void SetMessageCallback(MessageCallback cb) {
        msg_fn_ = cb;
    }
    void SetConnectionCallback(ConnectionCallback cb) {
        conn_fn_ = cb;
    }
    void SetCloseCallback(CloseCallback cb) {
        close_fn_ = cb;
    }
    void OnAttachedToLoop();
    /*std::string StatusToString() const;*/
private:
    void HandleRead();
    void HandleWrite();
    void HandleClose();
    // void DelayClose();
    void HandleError();
    void SendInLoop(const Slice& message);
    void SendInLoop(const void* data, size_t len);
    void SendStringInLoop(const std::string& message);

private:
    EventLoop* loop_;
    Socket_t fd_;
    uint64_t id_ = 0;
    std::string name_;
    std::string local_addr_; // the local address with form : "ip:port"
    std::string remote_addr_; // the remote address with form : "ip:port"
    std::unique_ptr<FdChannel> chan_;
    Buffer input_buffer_;
    Buffer output_buffer_; // TODO use a list<Slice> ??
    Type type_;
    
    volatile Status status_;
    size_t high_water_mark_ = 128 * 1024 * 1024; // Default 128MB

    ConnectionCallback conn_fn_; // This will be called to the user application layer
    MessageCallback msg_fn_; // This will be called to the user application layer
    WriteCompleteCallback write_complete_fn_; // This will be called to the user application layer
    HighWaterMarkCallback high_water_mark_fn_; // This will be called to the user application layer
    CloseCallback close_fn_; // This will be called to TCPClient or TCPServer
};
