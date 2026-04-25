#ifndef SK_UTILS_COROUTINE_SYNC_AWAIT_H
#define SK_UTILS_COROUTINE_SYNC_AWAIT_H

#include <cassert>
#include <condition_variable>
#include <coroutine>
#include <exception>
#include <mutex>
#include <optional>
#include <utility>

#include "task.h"

namespace sk::coro {

/// sync_await(task) — blocking wait for a Task<T> to complete.
///
/// Convenience for synchronously obtaining the result of a coroutine
/// from non-async code (e.g. main()). Internally, it resumes the task
/// and blocks via a condition variable until the task finishes.
///
/// Example:
///   int result = sync_await(compute_async());
namespace detail {

/// A simple thread-synchronization event
struct SyncEvent {
  std::mutex mtx;
  std::condition_variable cv;
  bool ready = false;

  void set() {
    {
      std::lock_guard<std::mutex> lock(mtx);
      ready = true;
    }
    cv.notify_one();
  }

  void wait() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] { return ready; });
  }
};

}  // namespace detail

/// sync_await for Task<T> (non-void)
template <typename T>
T sync_await(Task<T> task) {
  detail::SyncEvent event;
  std::optional<T> result;

  // Wrapper coroutine: await the task, store the result, signal the event.
  auto waiter = [&](Task<T> t) -> Task<void> {
    result = co_await t;
    event.set();
  };

  auto wait_task = waiter(std::move(task));
  // Start the wrapper (it is lazy — initial_suspend returns suspend_always)
  wait_task.handle().resume();

  // Block until the event is signaled
  event.wait();

  return std::move(result.value());
}

/// sync_await for Task<void>
inline void sync_await(Task<void> task) {
  detail::SyncEvent event;

  auto waiter = [&](Task<void> t) -> Task<void> {
    co_await t;
    event.set();
  };

  auto wait_task = waiter(std::move(task));
  wait_task.handle().resume();
  event.wait();
}

}  // namespace sk::coro

#endif  // SK_UTILS_COROUTINE_SYNC_AWAIT_H
