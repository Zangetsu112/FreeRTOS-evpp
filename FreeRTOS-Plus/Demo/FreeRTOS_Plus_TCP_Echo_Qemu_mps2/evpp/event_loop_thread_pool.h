#include "event_loop_thread.h"
#include "atomic.h"

#ifdef __cplusplus
    extern "C" {
#endif

class EventLoopThreadPool : public ServerStatus {
public:
    typedef std::function<void()> DoneCallback;
    // std::shared_ptr<std::atomic<uint32_t>> started_count(new std::atomic<uint32_t>(0));
    volatile int32_t started_count;
    // std::shared_ptr<std::atomic<uint32_t>> exited_count(new std::atomic<uint32_t>(0));
    volatile int32_t exited_count;

    EventLoopThreadPool(EventLoop* base_loop, uint32_t thread_num);
    ~EventLoopThreadPool();

    bool Start(bool wait_thread_started = false);

    void Stop(bool wait_thread_exited = false);
    void Stop(DoneCallback fn);

    // @brief Join all the working thread. If you forget to call this method,
    // it will be invoked automatically in the destruct method ~EventLoopThreadPool().
    // @note DO NOT call this method from any of the working thread.
    // void Join();

    // @brief Reinitialize some data fields after a fork
    // void AfterFork();
public:
    EventLoop* GetNextLoop();
    EventLoop* GetNextLoopWithHash(uint64_t hash);

    uint32_t thread_num() const;

private:
    void Stop(bool wait_thread_exit, DoneCallback fn);
    void OnThreadStarted(uint32_t count);
    void OnThreadExited(uint32_t count);

private:
    EventLoop* base_loop_;

    uint32_t thread_num_ = 0;
    // std::atomic<int64_t> next_ = { 0 
    volatile int32_t next_ = 0; 
    

    DoneCallback stopped_cb_;

    typedef std::shared_ptr<EventLoopThread> EventLoopThreadPtr;
    std::vector<EventLoopThreadPtr> threads_;
};

#ifdef __cplusplus
    }
#endif
