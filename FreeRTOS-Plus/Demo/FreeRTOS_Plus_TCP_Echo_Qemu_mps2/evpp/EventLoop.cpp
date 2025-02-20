#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "EventLoop.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"
#include <vector>
#include <map>
#include <functional>
#include "atomic.h"
// #include <atomic>
// #include <mutex>

// namespace evpp {
    bool PipeEventWatcher::createSocketPair() {
        struct freertos_sockaddr xBindAddress;
        const TickType_t xRxBlockTime_master = 0;

        xBindAddress.sin_family = FREERTOS_AF_INET;
        xBindAddress.sin_port = FreeRTOS_htons(0);

        
        xListeningSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
        if (xListeningSocket == FREERTOS_INVALID_SOCKET) return false;
        FreeRTOS_setsockopt(xListeningSocket, 0, FREERTOS_SO_RCVTIMEO, &xRxBlockTime_master, sizeof(TickType_t));

        if (FreeRTOS_bind(xListeningSocket, &xBindAddress, sizeof(xBindAddress)) != 0) {
            printf("Couldn't bind the listening socket \n");
            FreeRTOS_closesocket(xListeningSocket);
            return false;
        }

        FreeRTOS_GetLocalAddress(xListeningSocket, &xBindAddress);

        if (FreeRTOS_listen(xListeningSocket, 1) != 0) {
            printf("Couldn't listen on the listening socket \n");
            FreeRTOS_closesocket(xListeningSocket);
            return false;
        }
        return true;
    }

    void PipeEventWatcher::DoClose() {
    FreeRTOS_closesocket(xListeningSocket); 
    }

    PipeEventWatcher::PipeEventWatcher(EventLoop* loop, const Handler& handler) {
        loop_ = loop;
        evbase_ = loop -> event_base();
        handler_ = handler;
    }  

    PipeEventWatcher::PipeEventWatcher(EventLoop* loop, Handler&& handler) 
        : loop_(loop), handler_(std::move(handler)) {
           evbase_ = loop -> event_base(); 
        }

    PipeEventWatcher::~PipeEventWatcher() {
    DoClose(); 
    }

    bool PipeEventWatcher::Init() {
        if (!DoInit()) return false;
        return true;
    }

    bool PipeEventWatcher::DoInit() {
        if (!createSocketPair()) { 
            printf("Could not initialize Pipe Watcher\n");
            return false;
        }
        return true;
    }

    bool PipeEventWatcher::AsyncWait() {
        if (attached_) {
            bool ans = evbase_ -> deletePipeSocketEvent(xListeningSocket, eSELECT_INTR);
            configASSERT(ans);
            attached_ = false;
        }
        bool ans = evbase_ -> addPipeSocketEvent(xListeningSocket, handler_, eSELECT_INTR);
        configASSERT(ans); 
        attached_ = true;
        return true;
    }

    void PipeEventWatcher::Notify() {
        printf("Called Notify\n");
        // Send a signal to socket -- will cause Select to return an eSELECT_INTR
        if (FreeRTOS_SignalSocket(xListeningSocket) != 0) {
            printf("Couldn't send signal to socket -- check if it's active\n");
            configASSERT(false);
        }
    }

    Socket_t PipeEventWatcher::getServerSocket() { return xListeningSocket; }

    EventBase::EventBase() : socket_callback_count(0), exit_loop(false) {
        printf("Before Socketset initialization\n");
        xSocketSet = FreeRTOS_CreateSocketSet();
    }

    int EventBase::event_dispatch() {
        if (!socket_callback_count) // No event registered
            return 1;
        exit_loop = false;
        // Call necessary since the current version does not track interrupts on
        // pipe socket which were triggered before select
        pipe_callback();

        try {
            while(!exit_loop) {
                printf("Before Select is called\n");
                BaseType_t xResult = FreeRTOS_select(xSocketSet, portMAX_DELAY);
                // Watcher notify sends an interrupt signal
                if (xResult == eSELECT_INTR) {
                    printf("Custom event received from PipeWatcher\n");
                    pipe_callback();
                }

                else if (xResult > 0) {
                    for(auto pairs: socket_callbacks) {
                        if (FreeRTOS_FD_ISSET(pairs.first.first, xSocketSet)) {
                            pairs.second();
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            return -1;
        }
        return 0;
    }

    void EventBase::event_loopexit() {
        exit_loop = true;
        // Notify to breakout of select loop
        // watcher_ -> Notify();
    }

    bool EventBase::deleteSocketEvent(Socket_t socket, EventBits_t type) {
        FreeRTOS_FD_CLR(socket, xSocketSet, type);
        if (socket_callbacks.find(std::make_pair(socket, type)) == socket_callbacks.end())
            return false;
        socket_callbacks.erase(std::make_pair(socket, type));
        // socket_callback_count--;
        Atomic_Decrement_u32((uint32_t*)&socket_callback_count);
        return true;
    }

    bool EventBase::addPipeSocketEvent(Socket_t socket, std::function<void()> callback, EventBits_t type) {
        FreeRTOS_FD_SET(socket, xSocketSet, type);
        // socket_callback_count++; 
        Atomic_Increment_u32((uint32_t*)&socket_callback_count);
        pipe_callback = callback;
        return true;
    }

    bool EventBase::deletePipeSocketEvent(Socket_t socket, EventBits_t type) {
        FreeRTOS_FD_CLR(socket, xSocketSet, type);
        // socket_callback_count--;
        Atomic_Decrement_u32((uint32_t*)&socket_callback_count);
        pipe_callback = nullptr;
        return true;
    }

    bool EventBase::addSocketEvent(Socket_t socket, std::function<void()> callback, EventBits_t type) {
        if (socket_callback_count == 32) {
            printf("Cant handle more Events\n");
            return false;
        }

        // Assume events can only be of type 
        FreeRTOS_FD_SET(socket, xSocketSet, (EventBits_t) type);
        // socket_callback_count++; 
        Atomic_Increment_u32((uint32_t*)&socket_callback_count);
        if (callback) {
            socket_callbacks[std::make_pair(socket, type)] = callback;
        }
        return true;
    }

    // void start() {
    //     xTaskCreate(eventLoopTaskWrapper, "EventLoop", configMINIMAL_STACK_SIZE, this, tskIDLE_PRIORITY, NULL);
    // }

    EventLoop::EventLoop() {
        evbase_ = new EventBase();
        Init();
    }

    EventLoop::~EventLoop() {
        StopInLoop();
    } 

    void EventLoop::Init() {
        // status_.store(kInitializing);
        Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kInitializing);
        this->pending_functors_ = new std::vector<Functor>();
        mutex_ = xSemaphoreCreateMutex();

        InitNotifyPipeWatcher();
        // status_.store(kInitialized);
        Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kInitialized);
    }

    void EventLoop::InitNotifyPipeWatcher() {
        // Initialized task queue notify pipe watcher
        // watcher_.reset(new PipeEventWatcher(this, std::bind(&EventLoop::DoPendingFunctors, this)));
        watcher_.reset(new PipeEventWatcher(this, [this]() { this->DoPendingFunctors(); }));
        int rc = watcher_->Init();
        configASSERT(rc);
        if (!rc) {
            printf("ERROR: PipeEventWatcher init failed.\n");
        }
    }

    void EventLoop::Run() {
        // status_.store(kStarting);
        Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kStarting);
        int rc = watcher_->AsyncWait();
        configASSERT(rc);
        
        // After everything have initialized, we set the status to kRunning
        // status_.store(kRunning);
        Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kRunning); 

        rc = evbase_ -> event_dispatch();
        if (rc == 1) {
            printf("event_base_dispatch error: no event registered\n");
        } else if (rc == -1) {
            printf("event_base_dispatch error\n");
        }

        // Make sure watcher_ does construct, initialize and destruct in the same thread.
        watcher_.reset();
        printf("EventLoop stopped\n");

        // status_.store(kStopped);
        Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kStopped);
    }

    void EventLoop::Stop() {
        configASSERT(status_ == (void*)(intptr_t) kRunning);
        // status_.store(kStopping);
        Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kStopping);
        printf("EventLoop::Stop");
        // QueueInLoop(std::bind(&EventLoop::StopInLoop, this));
        QueueInLoop( [this]() { this->StopInLoop(); });
    }

    void EventLoop::StopInLoop() {
        configASSERT(status_ == (void*)(intptr_t) kStopping);

        auto f = [this]() {
            for (int i = 0;;i++) {
                printf("calling DoPendingFunctors index=%d", i);
                DoPendingFunctors();
                if (IsPendingQueueEmpty()) {
                    break;
                }
            }
        };

        f();
        evbase_ -> event_loopexit();
        watcher_ -> Notify(); // EventBase does not have acess to the watcher, we need to interrupt the select to exit out of waiting
        f();
    }


    void EventLoop::RunInLoop(const Functor& functor) {
        if (IsRunning() && IsInLoopThread()) {
            functor();
        } else {
            QueueInLoop(functor);
        }
    }

    void EventLoop::RunInLoop(Functor&& functor) {
        if (IsRunning() && IsInLoopThread()) {
            functor();
        } else {
            QueueInLoop(std::move(functor));
        }
    }

    void EventLoop::QueueInLoop(const Functor& cb) {
        int notif = reinterpret_cast<intptr_t>(notified_);;
        printf("pending_functor_count = %ld, PendingQueueSize = %d, notified = %d\n", 
                    pending_functor_count_, GetPendingQueueSize(), notif); 
        if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE){
            pending_functors_->emplace_back(cb);
            xSemaphoreGive(mutex_);
        }
        // ++pending_functor_count_;
        Atomic_Increment_u32((uint32_t*)&pending_functor_count_);
        if (!notified_) {
            printf("call watcher_->Nofity() notified_.store(true)\n");

            // We must set notified_ to true before calling `watcher_->Notify()`
            // otherwise there is a change that:
            //  1. We called watcher_- > Notify() on thread1
            //  2. On thread2 we watched this event, so wakeup the CPU changed to run this EventLoop on thread2 and executed all the pending task
            //  3. Then the CPU changed to run on thread1 and set notified_ to true
            //  4. Then, some thread except thread2 call this QueueInLoop to push a task into the queue, and find notified_ is true, so there is no change to wakeup thread2 to execute this task
            // notified_.store(true);
            Atomic_CompareAndSwapPointers_p32(&notified_, (void*)1, NULL);

            // Sometimes one thread invoke EventLoop::QueueInLoop(...), but anther
            // thread is invoking EventLoop::Stop() to stop this loop. At this moment
            // this loop maybe is stopping and the watcher_ object maybe has been
            // released already.
            if (watcher_) {
                watcher_->Notify();
            } else {
                configASSERT(!IsRunning());
            }
        } else {
            printf("No need to call watcher_->Nofity()\n");
        }
    }

    void EventLoop::QueueInLoop(Functor&& cb) {
        printf("pending_functor_count = %ld, PendingQueueSize = %d, notified = %d\n", 
                pending_functor_count_, GetPendingQueueSize(), notified_); 
        if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
            pending_functors_->emplace_back(std::move(cb));
            xSemaphoreGive(mutex_);
        }
        // ++pending_functor_count_;
        Atomic_Increment_u32((uint32_t*)&pending_functor_count_);

        printf("pending_functor_count = %ld, PendingQueueSize = %d, notified = %d\n", 
                pending_functor_count_, GetPendingQueueSize(), notified_);         
        if (!notified_) {
            printf("call watcher_->Nofity() notified_.store(true)\n");
            // notified_.store(true);
            Atomic_CompareAndSwapPointers_p32(&notified_, (void*)1, NULL);
            if (watcher_) {
                watcher_->Notify();
            } else {
                printf("watcher_ is empty, maybe we call EventLoop::QueueInLoop on a stopped EventLoop. status = %s", StatusToString());
                configASSERT(!IsRunning());
            }
        } else {
            printf("No need to call watcher_->Nofity()");
        }
    }

    void EventLoop::DoPendingFunctors() {
        printf("pending_functor_count = %ld, PendingQueueSize = %d, notified = %d\n",
                pending_functor_count_, GetPendingQueueSize(), notified_);
        std::vector<Functor> functors;
        if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE){
            // notified_.store(false)
            Atomic_CompareAndSwapPointers_p32(&notified_, NULL, (void*)1);
            pending_functors_->swap(functors);
            printf("pending_functor_count = %ld, PendingQueueSize = %d, notified = %d\n", 
                    pending_functor_count_, GetPendingQueueSize(), notified_);
            xSemaphoreGive(mutex_);
        }
        if (functors.size() == 0) return;
        for (size_t i = 0; i < functors.size(); ++i) {
            functors[i]();
            // --pending_functor_count_;
            Atomic_Decrement_u32((uint32_t*)&pending_functor_count_);
        }
        printf("pending_functor_count = %ld, PendingQueueSize = %d, notified = %d\n", 
                pending_functor_count_, GetPendingQueueSize(), notified_); 
    }

    int EventLoop::GetPendingQueueSize() {
        return pending_functors_ -> size();
    }

    bool EventLoop::IsPendingQueueEmpty() {
        return pending_functors_->empty();
    }
// }


//     class TestTask {
//     private:
//         static void taskWrapper(void* pvParameters) {
//             TestTask* instance = static_cast<TestTask*> (pvParameters);
//             instance -> run();
//     }

//     static void try_callback(void * none) {
//         printf("Recieved data on socket\n");
//     }

//     void run() {
//         printf("Before EventLoop\n");
//         EventLoop eventLoop;
        
//         // Test with a socket
//         struct freertos_sockaddr xBindAddress;
//         // Make socket non blocking
//         const TickType_t xRxBlockTime_master = 0;
//         xBindAddress.sin_family = FREERTOS_AF_INET;
//         xBindAddress.sin_port = FreeRTOS_htons(9999);

        
//         Socket_t socket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
//         // if (socket == FREERTOS_INVALID_SOCKET) return false;
//         FreeRTOS_setsockopt(socket, 0, FREERTOS_SO_RCVTIMEO, &xRxBlockTime_master, sizeof(TickType_t));

//         if (FreeRTOS_bind(socket, &xBindAddress, sizeof(xBindAddress)) != 0) {
//             printf("Couldn't bind the listening socket \n");
//             FreeRTOS_closesocket(socket);
//             // return false;
//         }

//         FreeRTOS_GetLocalAddress(socket, &xBindAddress);

//         if (FreeRTOS_listen(socket, 1) != 0) {
//             printf("Couldn't listen on the listening socket \n");
//             FreeRTOS_closesocket(socket);
//             // return false;
//         }

//         eventLoop.addSocketEvent(socket, try_callback);



//         eventLoop.start();
//         // std::map<int, int> my_map;
//         // my_map[1] = 2;
//         // my_map[4] = 5;
//         // printf("%d %d", my_map[1], my_map[4]);
//         // my_map.erase(1);
//         // auto it = my_map.find(1);
//         // if (it == my_map.end()) 
//         //     printf("1 removed from the map");
//         printf("Started Eventloop\n");


//         // Periodically trigger the PipeWatcher
//         while (1) {
//             vTaskDelay(pdMS_TO_TICKS(5000));  // Every 5 seconds
//             eventLoop.pipe_watcher->Notify();
//             printf("Sent a Notify to pipewatcher\n");
//         }
//     }

// public:
//     void start() {
//         xTaskCreate(taskWrapper, "TestTask", 2048, this, 1, NULL);
//     }

// };

// extern "C" void app_main(void) {
//     TestTask task;
//     task.start();
// }


