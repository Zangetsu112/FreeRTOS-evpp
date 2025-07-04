#include <unistd.h>
#include <sstream>
#include "event_loop_thread_pool.h"
#include "atomic.h"
#include "task.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop, uint32_t thread_number)
    : base_loop_(base_loop),
      thread_num_(thread_number) {
    // DLOG_TRACE << "thread_num=" << thread_num() << " base loop=" << base_loop_;
    // std::shared_ptr<std::atomic<uint32_t>> started_count(new std::atomic<uint32_t>(0));
    started_count = 0;
    // std::shared_ptr<std::atomic<uint32_t>> exited_count(new std::atomic<uint32_t>(0));
    exited_count = 0;
}

EventLoopThreadPool::~EventLoopThreadPool() {
    // DLOG_TRACE << "thread_num=" << thread_num();
    // Join();
    threads_.clear();
}

bool EventLoopThreadPool::Start(bool wait_thread_started) {
    // status_.store(kStarting);
    // Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kStarting);
    status_ = kStarting;
    // DLOG_TRACE << "thread_num=" << thread_num() << " base loop=" << base_loop_ << " wait_thread_started=" << wait_thread_started;

    if (thread_num_ == 0) {
        // status_.store(kRunning);
        // Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kRunning);
        status_ = kRunning;
        return true;
    } 

    for (uint32_t i = 0; i < thread_num_; ++i) {
        auto prefn = [this]() {
            // DLOG_TRACE << "a working thread started tid=" << std::this_thread::get_id();
            Atomic_Increment_u32((uint32_t*)&(this->started_count));
            this->OnThreadStarted(started_count);
            return EventLoopThread::kOK;
        };

        auto postfn = [this]() {
            // DLOG_TRACE << "a working thread exiting, tid=" << std::this_thread::get_id();
            Atomic_Increment_u32((uint32_t*)&(this->exited_count));
            this->OnThreadExited(exited_count);
            return EventLoopThread::kOK;
        };

        EventLoopThreadPtr t(new EventLoopThread());
        if (!t->Start(wait_thread_started, prefn, postfn)) {
            //FIXME error process
            // LOG_ERROR << "start thread failed!";
            return false;
        }

        std::stringstream ss;
        ss << "EventLoopThreadPool-thread-" << i << "th";
        // DLOG_TRACE << "New thread = " << ss.str();
        t->set_name(ss.str());
        threads_.push_back(t);
    }

    // when all the working thread have started,
    // status_ will be stored with kRunning in method OnThreadStarted

    if (wait_thread_started) {
        while (!IsRunning()) {
            // usleep(pdMS_TO_TICKS(1000));
            taskYIELD();
        }
        configASSERT(status_ == kRunning);
        // configASSERT(static_cast<ServerStatus::Status>(reinterpret_cast<intptr_t>(status_)) == ServerStatus::Status::kRunning);
    }

    return true;
}

void EventLoopThreadPool::Stop(bool wait_thread_exit) {
    // DLOG_TRACE << "wait_thread_exit=" << wait_thread_exit;
    Stop(wait_thread_exit, DoneCallback());
}

void EventLoopThreadPool::Stop(DoneCallback fn) {
    // DLOG_TRACE;
    Stop(false, fn);
}

void EventLoopThreadPool::Stop(bool wait_thread_exit, DoneCallback fn) {
    // DLOG_TRACE;
    // status_.store(kStopping);
    // Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kStopping);
    status_ = kStopping;
    
    if (thread_num_ == 0) {
        // status_.store(kStopping);
        // Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kStopping);
        status_ = kStopping;
        
        if (fn) {
            // DLOG_TRACE << "calling stopped callback";
            fn();
        }
        return;
    }

    // DLOG_TRACE << "wait_thread_exit=" << wait_thread_exit;
    stopped_cb_ = fn;

    for (auto &t : threads_) {
        t->Stop();
    }

    // when all the working thread have stopped
    // status_ will be stored with kStopped in method OnThreadExited

    auto is_stopped_fn = [this]() {
        for (auto &t : this->threads_) {
            if (!t->IsStopped()) {
                return false;
            }
        }
        return true;
    };

    // DLOG_TRACE << "before promise wait";
    if (thread_num_ > 0 && wait_thread_exit) {
        while (!is_stopped_fn()) {
            // usleep(pdMS_TO_TICKS(1000));
            taskYIELD();
        }
    }
    // DLOG_TRACE << "after promise wait";

    // status_.store(kStopped);
    // Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kStopped);
    status_ = kStopped;
}

// void EventLoopThreadPool::Join() {
//     // DLOG_TRACE << "thread_num=" << thread_num();
//     for (auto &t : threads_) {
//         t->Join();
//     }
//     threads_.clear();
// }

// void EventLoopThreadPool::AfterFork() {
//     // DLOG_TRACE << "thread_num=" << thread_num();
//     for (auto &t : threads_) {
//         t->AfterFork();
//     }
// }

EventLoop* EventLoopThreadPool::GetNextLoop() {
    // DLOG_TRACE;
    EventLoop* loop = base_loop_;

    if (IsRunning() && !threads_.empty()) {
        // No need to lock here
        Atomic_Increment_u32((uint32_t*)&next_);
        int64_t next = next_;
        next = next % threads_.size();
        loop = (threads_[next])->loop();
    }

    return loop;
}

EventLoop* EventLoopThreadPool::GetNextLoopWithHash(uint64_t hash) {
    EventLoop* loop = base_loop_;

    if (IsRunning() && !threads_.empty()) {
        uint64_t next = hash % threads_.size();
        loop = (threads_[next])->loop();
    }

    return loop;
}

uint32_t EventLoopThreadPool::thread_num() const {
    return thread_num_;
}

void EventLoopThreadPool::OnThreadStarted(uint32_t count) {
    // DLOG_TRACE << "tid=" << std::this_thread::get_id() << " count=" << count << " started.";
    if (count == thread_num_) {
        // DLOG_TRACE << "thread pool totally started.";
        // status_.store(kRunning);
        // Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kRunning);
        status_ = kRunning;
    }
}

void EventLoopThreadPool::OnThreadExited(uint32_t count) {
    // DLOG_TRACE << "tid=" << std::this_thread::get_id() << " count=" << count << " exited.";
    if (count == thread_num_) {
        // status_.store(kStopped);
        // Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kStopped);
        status_ = kStopped;
        // DLOG_TRACE << "this is the last thread stopped. Thread pool totally exited.";
        if (stopped_cb_) {
            stopped_cb_();
            stopped_cb_ = DoneCallback();
        }
    }
}
