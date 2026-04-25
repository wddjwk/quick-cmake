#ifndef SK_UTILS_COROUTINE_EVENT_LOOP_H
#define SK_UTILS_COROUTINE_EVENT_LOOP_H

#include <cassert>
#include <condition_variable>
#include <coroutine>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace sk::coro {

/// A simple single-threaded event loop for driving coroutines.
///
/// The loop maintains a queue of callable items (usually coroutine
/// resumers). On each iteration it drains the queue, executing each
/// item. This is the fundamental scheduler primitive — async
/// operations post their completions here, and the loop resumes the
/// waiting coroutines.
///
/// Usage:
///   EventLoop loop;
///   loop.post([]{ ... });        // schedule a function
///   loop.run();                  // block until no more work
class EventLoop {
  public:
  using Task = std::function<void()>;

  EventLoop() = default;

  EventLoop(const EventLoop&) = delete;
  EventLoop& operator=(const EventLoop&) = delete;

  /// Enqueue a callable to be executed on the next loop iteration.
  void post(Task task) {
    {
      std::lock_guard<std::mutex> lock(mtx_);
      queue_.push(std::move(task));
    }
    cv_.notify_one();
  }

  /// Run the loop until the queue is empty AND no more work is expected.
  /// This blocks the calling thread.
  void run() {
    running_ = true;
    while (running_) {
      std::queue<Task> local;
      {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return !queue_.empty() || !running_; });
        std::swap(local, queue_);
      }
      while (!local.empty()) {
        auto& task = local.front();
        if (task) {
          task();
        }
        local.pop();
      }
    }
  }

  /// Signal the loop to stop.
  void stop() {
    {
      std::lock_guard<std::mutex> lock(mtx_);
      running_ = false;
    }
    cv_.notify_one();
  }

  /// Check whether the loop is currently running.
  bool is_running() const { return running_; }

  private:
  std::queue<Task> queue_;
  mutable std::mutex mtx_;
  std::condition_variable cv_;
  bool running_ = false;
};

/// A global event loop singleton (convenience for simple programs).
/// Real applications should create their own EventLoop instance.
inline EventLoop& global_event_loop() {
  static EventLoop loop;
  return loop;
}

}  // namespace sk::coro

#endif  // SK_UTILS_COROUTINE_EVENT_LOOP_H
