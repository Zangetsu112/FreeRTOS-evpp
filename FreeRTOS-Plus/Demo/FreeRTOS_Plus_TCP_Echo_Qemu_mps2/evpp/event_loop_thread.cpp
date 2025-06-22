#include <unistd.h>
#include <sstream>
#include "EventLoop.h"
#include "event_loop_thread.h"
#include "task.h"
#include "FreeRTOSIPConfig.h"

// namespace evpp {

EventLoopThread::EventLoopThread()
    : event_loop_(new EventLoop) {
}

EventLoopThread::~EventLoopThread() {
    printf("loop = %p", event_loop_.get());
}

bool EventLoopThread::Start(bool wait_thread_started, Functor pre, Functor post) {
    status_ = kStarting;
    // Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kStarting);

    configASSERT(thread_ == nullptr);
    // thread_.reset(new std::thread(std::bind(&EventLoopThread::Run, this, pre, post)));
    // Store pre and post callbacks
    pre_ = pre;
    post_ = post;
    // Create the task
    BaseType_t result_ = xTaskCreate(
        taskFunction,        // Static function to run
        name_.c_str(),
        configMINIMAL_STACK_SIZE, // Stack size (adjust as needed)
        this,                // Pass 'this' pointer as parameter
        2,    // Task priority
        &thread_ // Task handle
    );
    // We do this alloc so the EventLoop can tell if it is running in a task (thread)
    // Change this - this is a horrible way of doing it 
    /*event_loop_->thread_id = uxTaskGetTaskNumber(thread_);*/
    TaskStatus_t xTaskDetails;
    vTaskGetInfo(thread_, &xTaskDetails, 1, eRunning);
    event_loop_->thread_id = xTaskDetails.xTaskNumber;


    if (result_ != pdPASS) {
        FreeRTOS_debug_printf(("Couldn't start task\n"));
    }

    if (wait_thread_started) {
        // while (reinterpret_cast<intptr_t>(status_) < kRunning) {
        while (status_ < kRunning) {
            // usleep(pdMS_TO_TICKS(1000));
            taskYIELD();
        }
    }
    return true;
}

void EventLoopThread::Run(const Functor& pre, const Functor& post) {
    if (name_.empty()) {
        std::ostringstream os;
        // os << "thread-" << uxTaskGetTaskNumber(thread_); // get task handle number
        name_ = os.str();
    }

    auto fn = [this, pre]() {
        status_ = kRunning;
        // Atomic_SwapPointers_p32(&this->status_, (void*)(intptr_t) Status::kRunning);
        if (pre) {
            auto rc = pre();
            if (rc != kOK) {
                this->event_loop_->Stop();
            }
        }
    };
    this->event_loop_->QueueInLoop(std::move(fn));
    this->event_loop_->Run();

    if (post) {
        post();
    }

    status_ = kStopped;
    // Atomic_SwapPointers_p32(&this->status_, (void*)(intptr_t) Status::kStopped);
}

void EventLoopThread::Stop(bool wait_thread_exit) {
    // configASSERT(reinterpret_cast<intptr_t>(status_) == kRunning && IsRunning());
    configASSERT(status_ == kRunning && IsRunning());
    status_ = kStopping;
    // Atomic_SwapPointers_p32(&status_, (void*)(intptr_t) Status::kStopping);
    event_loop_->Stop();

    if (wait_thread_exit) {
        while (!IsStopped()) {
            /*usleep(pdMS_TO_TICKS(1000));*/
            taskYIELD();
        }

        if (thread_ != nullptr) {
                vTaskDelete(thread_);
                thread_ = nullptr;
        }
    }
}

// void EventLoopThread::Join() {
//     // To avoid multi other threads call Join simultaneously
//     std::lock_guard<std::mutex> guard(mutex_);
//     if (thread_ && thread_->joinable()) {
//         DLOG_TRACE << "loop=" << event_loop_ << " thread=" << thread_ << " joinable";
//         try {
//             thread_->join();
//         } catch (const std::system_error& e) {
//             LOG_ERROR << "Caught a system_error:" << e.what() << " code=" << e.code();
//         }
//         thread_.reset();
//     }
// }

void EventLoopThread::set_name(const std::string& n) {
    name_ = n;
}

const std::string& EventLoopThread::name() const {
    return name_;
}


EventLoop* EventLoopThread::loop() const {
    return event_loop_.get();
}


EventBase* EventLoopThread::event_base() {
    return loop()->event_base();
}

UBaseType_t EventLoopThread::tid() const {
    if (thread_) {
        // return thread_->get_id();
        return uxTaskGetTaskNumber(thread_);
    }

    // return std::thread::id();
    return uxTaskGetTaskNumber(xTaskGetCurrentTaskHandle());
}

bool EventLoopThread::IsRunning() const {
    // Using event_loop_->IsRunning() is more exact to query where thread is
    // running or not instead of status_ == kRunning
    //
    // Because in some particular circumstances,
    // when status_==kRunning and event_loop_::running_ == false,
    // the application will broke down
    return event_loop_->IsRunning();
}

// }
