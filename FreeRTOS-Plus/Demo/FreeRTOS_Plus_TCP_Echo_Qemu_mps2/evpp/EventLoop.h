#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOSConfig.h"
#include "server_status.h"
#include "semphr.h"
#include "inner_pre.h"
#include <map>
#include <vector>
// #include <atomic>
#include "atomic.h"
#include <functional>
#include <memory>

#ifdef __cplusplus
    extern "C" {
#endif

// namespace evpp {
    class EventLoop;
    class EventBase; // Forward Declaration

    class PipeEventWatcher {
    public:
        typedef std::function<void()> Handler;

        PipeEventWatcher(EventLoop* loop, const Handler& handler);
        PipeEventWatcher(EventLoop* loop, Handler&& handler);
        ~PipeEventWatcher();

        bool AsyncWait();
        void Notify();

        bool Init();

        // @note It MUST be called in the event thread.
        void Cancel();

        void ClearHandler() { handler_ = Handler(); }

    protected:
        bool createSocketPair();
        void Close();

        bool DoInit();
        void DoClose();
        Socket_t getServerSocket();
        // static void HandlerFn(Socket_t fd, short which, void* v);

    protected:
        Socket_t xListeningSocket;
        bool attached_;
        Handler handler_;
        Handler cancel_callback_;
        EventBase* evbase_;
        EventLoop* loop_;
    };


    class EventBase {
    private:
        SocketSet_t xSocketSet;
        // std::atomic<int> socket_callback_count;
        volatile int32_t socket_callback_count = 0; // FreeRTOS atomic int
        bool exit_loop;
        std::function<void()> pipe_callback;
        std::map<std::pair<Socket_t, EventBits_t>, std::function<void()>> socket_callbacks;

    public:
        EventBase();
        int event_dispatch(); 
        bool deleteSocketEvent(Socket_t socket, EventBits_t type);
        bool addPipeSocketEvent(Socket_t socket, std::function<void()> callback, EventBits_t type);
        bool deletePipeSocketEvent(Socket_t socket, EventBits_t type);
        bool addSocketEvent(Socket_t socket, std::function<void()> callback, EventBits_t type);
        void event_loopexit();
    };

    class EventLoop: public ServerStatus {
    public:
        typedef std::function<void()> Functor;
        UBaseType_t thread_id = 0;
        EventBase* evbase_;
        EventLoop();
        ~EventLoop();


        // @brief Run the IO Event driving loop forever
        // @note It must be called in the IO Event thread
        void Run();

        // @brief Stop the event loop
        void Stop();

        void RunInLoop(const Functor& handler);
        void QueueInLoop(const Functor& handler);

        void RunInLoop(Functor&& handler);
        void QueueInLoop(Functor&& handler);

        // Getter and Setter
        EventBase* event_base() {
            return evbase_;
        }
        bool IsInLoopThread() const {
            // return xTaskHandle != nullptr;
            return thread_id == 0;
        }
        int pending_functor_count() const {
            return pending_functor_count_;
        }
        UBaseType_t tid() const {
            return thread_id;
        }

    private:

        void Init();
        void InitNotifyPipeWatcher();
        void StopInLoop();
        void DoPendingFunctors();
        int GetPendingQueueSize();
        bool IsPendingQueueEmpty();

        SemaphoreHandle_t mutex_;
        // We use this to notify the thread when we put a task into the pending_functors_ queue
        std::shared_ptr<PipeEventWatcher> watcher_;
        // When we put a task into the pending_functors_ queue,
        // we need to notify the thread to execute it. But we don't want to notify repeatedly.
        // std::atomic<bool> notified_;
        void* volatile notified_ = NULL; // FreeRTOS atomic bool
        std::vector<Functor>* pending_functors_; // @Guarded By mutex_
        // std::atomic<int> pending_functor_count_;
        volatile int32_t pending_functor_count_ = 0; // FreeRTOS atomic int
    };
// }

// void app_main();

#ifdef __cplusplus
    }
#endif

#endif
