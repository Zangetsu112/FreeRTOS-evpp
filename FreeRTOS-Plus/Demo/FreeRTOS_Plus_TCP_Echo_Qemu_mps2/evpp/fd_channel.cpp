#include "EventLoop.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "fd_channel.h"
#include <functional>
#include <string>

FdChannel::FdChannel(EventLoop* l, Socket_t f, bool r, bool w)
    : loop_(l), attached_(false), fd_(f) {
    events_ = (r ? kReadable : 0) | (w ? kWritable : 0);
}

FdChannel::~FdChannel() {
    Close();
}

void FdChannel::Close() {
    if (attached_) {
        DetachFromLoop();
    }
    read_fn_ = ReadEventCallback();
    write_fn_ = EventCallback();
}

void FdChannel::AttachToLoop() {
    if (!IsNoneEvent() && !attached_) {
        // loop_->evbase_->addSocketEvent(fd_, std::bind(&FdChannel::HandleEvent, this), events_);
        loop_->evbase_->addSocketEvent(fd_, [this]() { this->HandleEvent(); }, events_);
        attached_ = true;
    }
}

void FdChannel::EnableReadEvent() {
    int events = events_;
    events_ |= kReadable;
    if (events_ != events) {
        Update();
    }
}

void FdChannel::EnableWriteEvent() {
    int events = events_;
    events_ |= kWritable;
    if (events_ != events) {
        Update();
    }
}

void FdChannel::DisableReadEvent() {
    int events = events_;
    events_ &= (~kReadable);
    if (events_ != events) {
        Update();
    }
}

void FdChannel::DisableWriteEvent() {
    int events = events_;
    events_ &= (~kWritable);
    if (events_ != events) {
        Update();
    }
}

void FdChannel::DisableAllEvent() {
    if (events_ != kNone) {
        events_ = kNone;
        Update();
    }
}

void FdChannel::DetachFromLoop() {
    if (attached_) {
        loop_->evbase_->deleteSocketEvent(fd_, events_);
        attached_ = false;
    }
}

void FdChannel::Update() {
    if (IsNoneEvent()) {
        DetachFromLoop();
    } else {
        AttachToLoop();
    }
}


void FdChannel::HandleEvent() {
    if ((events_ & kReadable) && read_fn_) {
        read_fn_();
    }
    if ((events_ & kWritable) && write_fn_) {
        write_fn_();
    }
}

    // EventLoop* loop_;
    // bool attached_;
    // Socket_t fd_;
    // int events_;
    // ReadEventCallback read_fn_;
    // EventCallback write_fn_;