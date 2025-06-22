#ifndef SERVERSTATUS_H
#define SERVERSTATUS_H

#include <string>
// #include <atoimic>
#include "atomic"

#ifndef H_CASE_STRING_BIGIN
#define H_CASE_STRING_BIGIN(state) switch(state){
#define H_CASE_STRING(state) case state:return #state;break;
#define H_CASE_STRING_END()  default:return "Unknown";break;}
#endif
#ifdef __cplusplus
    extern "C" {
#endif

// namespace evpp {
    class ServerStatus {
    public:
        enum Status {
            kNull = 0,
            kInitializing = 1,
            kInitialized = 2,
            kStarting = 3,
            kRunning = 4,
            kStopping = 5,
            kStopped = 6,
        };

        enum SubStatus {
            kSubStatusNull = 0,
            kStoppingListener = 1,
            kStoppingThreadPool = 2,
        };

        std::string StatusToString() const {
            H_CASE_STRING_BIGIN(status_);
            H_CASE_STRING(kNull);
            H_CASE_STRING(kInitialized);
            H_CASE_STRING(kRunning);
            H_CASE_STRING(kStopping);
            H_CASE_STRING(kStopped);
            H_CASE_STRING_END();
        }

        bool IsRunning() const {
            return status_ == kRunning;
        }

        bool IsStopped() const {
            return status_ == kStopped;
        }

        bool IsStopping() const {
            return status_ == kStopping;
        }

    protected:
        // std::atomic<Status> status_ = { kNull };
        // std::atomic<SubStatus> substatus_ = { kSubStatusNull };
        // void* volatile status_ = 0;
        // void* volatile substatus_ = 0;
        volatile uintptr_t status_ = kNull;      // Atomic status management
        volatile uintptr_t substatus_ = kSubStatusNull; 
    };
// }

#ifdef __cplusplus
    }
#endif

#endif
