#include "tcp_server.h"
#include "listener.h"
#include "tcp_conn.h"
#include "atomic.h"
#include "EventLoop.h"

TCPServer::TCPServer(EventLoop* loop,
                     const std::string& laddr,
                     const std::string& name,
                     uint32_t thread_num)
    : loop_(loop)
    , listen_addr_(laddr)
    , name_(name)
    , conn_fn_(&internal::DefaultConnectionCallback)
    , msg_fn_(&internal::DefaultMessageCallback)
    , next_conn_id_(0) {
    // DLOG_TRACE << "name=" << name << " listening addr " << laddr << " thread_num=" << thread_num;
    tpool_.reset(new EventLoopThreadPool(loop_, thread_num));
}

TCPServer::~TCPServer() {
    // DLOG_TRACE;
    configASSERT(connections_.empty());
    configASSERT(!listener_);
    if (tpool_) {
        configASSERT(tpool_->IsStopped());
        tpool_.reset();
    }
}

bool TCPServer::Init() {
    // DLOG_TRACE;
    configASSERT(static_cast<ServerStatus::Status>(reinterpret_cast<intptr_t>(status_)) == kNull);
    listener_.reset(new Listener(loop_, listen_addr_));
    listener_->Listen();
    // status_.store(kInitialized);
    Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kInitialized);
    return true;
}

bool TCPServer::Start() {
    // DLOG_TRACE;
    configASSERT(static_cast<ServerStatus::Status>(reinterpret_cast<intptr_t>(status_)) == kInitialized);
    // status_.store(kStarting);
    Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kStarting);
    configASSERT(listener_.get());
    bool rc = tpool_->Start(true);
    if (rc) {
        configASSERT(tpool_->IsRunning());
        listener_->SetNewConnectionCallback(
            // std::bind( &TCPServer::HandleNewConn,
            //           this,
            //           std::placeholders::_1,
            //           std::placeholders::_2,
            //           std::placeholders::_3)
            // );
            
            [this](Socket_t sockfd, const std::string& remote_addr, const struct freertos_sockaddr* raddr) {
                this->HandleNewConn(sockfd, remote_addr, raddr);
            }
        );

        // We must set status_ to kRunning firstly and then we can accept new
        // connections. If we use the following code :
        //     listener_->Accept();
        //     status_.store(kRunning);
        // there is a chance : we have accepted a connection but status_ is not
        // kRunning that will cause the configASSERT(status_ == kRuning) failed in
        // TCPServer::HandleNewConn.
        // status_.store(kRunning);
        Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kRunning);
        listener_->Accept();
    }
    return rc;
}

void TCPServer::Stop(DoneCallback on_stopped_cb) {
    // DLOG_TRACE << "Entering ...";
    configASSERT(static_cast<ServerStatus::Status>(reinterpret_cast<intptr_t>(status_)) == kRunning);
    // status_.store(kStopping);
    Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kStopping);
    // substatus_.store(kStoppingListener);
    Atomic_SwapPointers_p32(&substatus_, (void*)(intptr_t) SubStatus::kStoppingListener); 
    // loop_->RunInLoop(std::bind(&TCPServer::StopInLoop, this, on_stopped_cb));
    loop_->RunInLoop([this, on_stopped_cb]() { this->StopInLoop(on_stopped_cb); });
}

void TCPServer::StopInLoop(DoneCallback on_stopped_cb) {
    // DLOG_TRACE << "Entering ...";
    configASSERT(loop_->IsInLoopThread());
    listener_->Stop();
    listener_.reset();

    if (connections_.empty()) {
        // Stop all the working threads now.
        // DLOG_TRACE << "no connections";
        StopThreadPool();
        if (on_stopped_cb) {
            on_stopped_cb();
            on_stopped_cb = DoneCallback();
        }
        // status_.store(kStopped);
        Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kStopped);
    } else {
        // DLOG_TRACE << "close connections";
        for (auto& c : connections_) {
            if (c.second->IsConnected()) {
                // DLOG_TRACE << "close connection id=" << c.second->id() << " fd=" << c.second->fd();
                c.second->Close();
            } else {
                // DLOG_TRACE << "Do not need to call Close for this TCPConn it may be doing disconnecting. TCPConn=" << c.second.get() << " fd=" << c.second->fd() << " status=" << StatusToString();
            }
        }

        stopped_cb_ = on_stopped_cb;

        // The working threads will be stopped after all the connections closed.
    }

    // DLOG_TRACE << "exited, status=" << StatusToString();
}

void TCPServer::StopThreadPool() {
    // DLOG_TRACE << "pool=" << tpool_.get();
    configASSERT(loop_->IsInLoopThread());
    configASSERT(IsStopping());
    // substatus_.store(kStoppingThreadPool);
    Atomic_SwapPointers_p32(&substatus_, (void*)(intptr_t) SubStatus::kStoppingThreadPool);
    tpool_->Stop(true);
    configASSERT(tpool_->IsStopped());

    // Make sure all the working threads totally stopped.
    // tpool_->Join();
    tpool_.reset();

    // substatus_.store(kSubStatusNull);
    Atomic_SwapPointers_p32(&substatus_, (void*)(intptr_t) SubStatus::kSubStatusNull);
}

void TCPServer::HandleNewConn(Socket_t sockfd,
                              const std::string& remote_addr/*ip:port*/,
                              const struct freertos_sockaddr* raddr) {
    // DLOG_TRACE << "fd=" << sockfd;
    configASSERT(loop_->IsInLoopThread());
    if (IsStopping()) {
        // LOG_WARN << "this=" << this << " The server is at stopping status. Discard this socket fd=" << sockfd << " remote_addr=" << remote_addr;
        FreeRTOS_closesocket(sockfd);
        return;
    }

    configASSERT(IsRunning());
    EventLoop* io_loop = GetNextLoop(raddr);
    std::string n = remote_addr;
    ++next_conn_id_;
    TCPConnPtr conn(new TCPConn(io_loop, n, sockfd, listen_addr_, remote_addr, next_conn_id_));
    configASSERT(conn->type() == TCPConn::kIncoming);
    conn->SetMessageCallback(msg_fn_);
    conn->SetConnectionCallback(conn_fn_);
    // conn->SetCloseCallback(std::bind(&TCPServer::RemoveConnection, this, std::placeholders::_1));
    conn->SetCloseCallback([this](const TCPConnPtr& connection) {this-> RemoveConnection(connection); });
    // io_loop->RunInLoop(std::bind(&TCPConn::OnAttachedToLoop, conn));
    io_loop->RunInLoop([conn]() { conn -> OnAttachedToLoop(); });
    connections_[conn->id()] = conn;
}

EventLoop* TCPServer::GetNextLoop(const struct freertos_sockaddr* raddr) {
    if (IsRoundRobin()) {
        return tpool_->GetNextLoop();
    } else {
        return tpool_->GetNextLoopWithHash(raddr->sin_address.ulIP_IPv4);
    }
}

void TCPServer::RemoveConnection(const TCPConnPtr& conn) {
    // DLOG_TRACE << "conn=" << conn.get() << " fd="<< conn->fd() << " connections_.size()=" << connections_.size();
    auto f = [this, conn]() {
        // Remove the connection in the listening EventLoop
        // DLOG_TRACE << "conn=" << conn.get() << " fd="<< conn->fd() << " connections_.size()=" << connections_.size();
        configASSERT(this->loop_->IsInLoopThread());
        this->connections_.erase(conn->id());
        if (IsStopping() && this->connections_.empty()) {
            // At last, we stop all the working threads
            // DLOG_TRACE << "stop thread pool";
            // configASSERT(substatus_ == kStoppingListener);
            configASSERT(static_cast<ServerStatus::SubStatus>(reinterpret_cast<intptr_t>(substatus_)) == ServerStatus::SubStatus::kStoppingListener);
            StopThreadPool();
            if (stopped_cb_) {
                stopped_cb_();
                stopped_cb_ = DoneCallback();
            }
            // status_.store(kStopped);
            Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kStopped);
        }
    };
    loop_->RunInLoop(f);
}