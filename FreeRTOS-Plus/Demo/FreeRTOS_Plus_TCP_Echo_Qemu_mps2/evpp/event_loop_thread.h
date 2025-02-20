#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "queue.h"
#include "semphr.h"
#include "server_status.h"
#include "task.h"
// #include "inner_pre.h"
// #include <atomic>
#include "EventLoop.h"
#include <functional>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

// namespace evpp {
class EventLoopThread : public ServerStatus {
private:
  std::function<int()> pre_ = nullptr;
  std::function<int()> post_ = nullptr;

  static void taskFunction(void *pvParameters) {
    EventLoopThread *self = static_cast<EventLoopThread *>(pvParameters);
    self->Run(self->pre_, self->post_);
    vTaskDelete(nullptr); // Delete the task when Run() completes
  }

public:
  enum { kOK = 0 };

  // Return 0 means OK, anything else means failed.
  typedef std::function<int()> Functor;

  EventLoopThread();
  ~EventLoopThread();

  // @param wait_thread_started - If it is true this method will block
  //  until the thread totally started
  // @param pre - This functor will be executed immediately when the thread is
  // started.
  // @param post - This functor will be executed at the moment when the thread
  // is going to stop.
  bool Start(bool wait_thread_started = true, Functor pre = Functor(),
             Functor post = Functor());

  void Stop(bool wait_thread_exit = false);

public:
  void set_name(const std::string &n);
  const std::string &name() const;
  EventLoop *loop() const;
  EventBase *event_base();
  // std::thread::id tid() const;
  UBaseType_t tid() const;
  bool IsRunning() const;

private:
  void Run(const Functor &pre, const Functor &post);

  std::shared_ptr<EventLoop> event_loop_;
  TaskHandle_t thread_ = nullptr;
  std::string name_;
};
// }

#ifdef __cplusplus
}
#endif
