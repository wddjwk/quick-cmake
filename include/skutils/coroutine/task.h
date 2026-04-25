#ifndef SK_UTILS_COROUTINE_TASK_H
#define SK_UTILS_COROUTINE_TASK_H

#include <atomic>
#include <coroutine>
#include <exception>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace sk::coro {

/// Forward declaration
template <typename T>
class Task;

namespace detail {

/// Type trait to check if T is a Task<>
template <typename T>
struct IsTask : std::false_type {};

template <typename U>
struct IsTask<Task<U>> : std::true_type {};

template <typename T>
inline constexpr bool IsTaskV = IsTask<T>::value;

// ============================================================
// TaskPromiseBase — shared logic for Task<T> and Task<void>
// ============================================================
class TaskPromiseBase {
  public:
  /// The continuation (caller coroutine) to resume when this task completes.
  std::coroutine_handle<> continuation_ = nullptr;

  /// Lazy start: the coroutine does not begin executing until explicitly awaited.
  auto initial_suspend() { return std::suspend_always{}; }

  void unhandled_exception() { exception_ = std::current_exception(); }

  protected:
  std::exception_ptr exception_;
};

// ============================================================
// TaskFinalAwaiter — resumes the caller after the callee finishes
// ============================================================
struct FinalAwaiter {
  bool await_ready() noexcept { return false; }

  template <typename Promise>
  std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> handle) noexcept {
    // Return the caller's continuation so the scheduler resumes it.
    // If there is no continuation, we return noop (no-op) so the
    // runtime simply returns to the resumer.
    auto& promise = handle.promise();
    if (promise.continuation_) {
      return promise.continuation_;
    }
    return std::noop_coroutine();
  }

  void await_resume() noexcept {}
};

}  // namespace detail

// ============================================================
// Task<T> — an eager-when-awaited coroutine that produces a value
// ============================================================
/// Task<T> models an asynchronous computation that eventually produces
/// a value of type T. It supports:
///   - co_return value       (produce the final result)
///   - co_await other_task   (compose / chain async operations)
///   - co_await awaitable    (integrate with any Awaitable)
///
/// The coroutine starts lazily (initial_suspend returns suspend_always)
/// and is resumed only when someone co_awaits or sync_awaits it.
template <typename T>
class Task {
  public:
  struct promise_type;
  using HandleType = std::coroutine_handle<promise_type>;

  struct promise_type : public detail::TaskPromiseBase {
    T value_;
    bool has_value_ = false;

    Task get_return_object() { return Task{HandleType::from_promise(*this)}; }

    auto final_suspend() noexcept { return detail::FinalAwaiter{}; }

    void return_value(T value) {
      value_ = std::move(value);
      has_value_ = true;
    }

    T& result() & {
      if (exception_) {
        std::rethrow_exception(exception_);
      }
      return value_;
    }

    T&& result() && {
      if (exception_) {
        std::rethrow_exception(exception_);
      }
      return std::move(value_);
    }
  };

  // --- Awaitable interface ---
  // When someone co_awaits a Task<T>, the following three functions
  // control the suspension / resumption behavior.

  bool await_ready() noexcept { return false; }

  std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
    // Store the caller as our continuation, then start executing this task.
    handle_.promise().continuation_ = caller;
    return handle_;
  }

  T await_resume() { return std::move(handle_.promise().result()); }

  // --- Construction / move / destruction ---

  explicit Task(HandleType handle) noexcept : handle_(handle) {}

  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;

  Task(Task&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }

  Task& operator=(Task&& other) noexcept {
    if (this != &other) {
      if (handle_) {
        handle_.destroy();
      }
      handle_ = other.handle_;
      other.handle_ = nullptr;
    }
    return *this;
  }

  ~Task() {
    if (handle_) {
      handle_.destroy();
    }
  }

  /// Directly access the underlying coroutine handle (advanced use).
  HandleType handle() const noexcept { return handle_; }

  private:
  HandleType handle_;
};

// ============================================================
// Task<void> — specialization for void-returning coroutines
// ============================================================
template <>
class Task<void> {
  public:
  struct promise_type;
  using HandleType = std::coroutine_handle<promise_type>;

  struct promise_type : public detail::TaskPromiseBase {
    Task get_return_object() { return Task{HandleType::from_promise(*this)}; }

    auto final_suspend() noexcept { return detail::FinalAwaiter{}; }

    void return_void() {}

    void result() {
      if (exception_) {
        std::rethrow_exception(exception_);
      }
    }
  };

  bool await_ready() noexcept { return false; }

  std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
    handle_.promise().continuation_ = caller;
    return handle_;
  }

  void await_resume() { handle_.promise().result(); }

  explicit Task(HandleType handle) noexcept : handle_(handle) {}

  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;

  Task(Task&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }

  Task& operator=(Task&& other) noexcept {
    if (this != &other) {
      if (handle_) {
        handle_.destroy();
      }
      handle_ = other.handle_;
      other.handle_ = nullptr;
    }
    return *this;
  }

  ~Task() {
    if (handle_) {
      handle_.destroy();
    }
  }

  HandleType handle() const noexcept { return handle_; }

  private:
  HandleType handle_;
};

// ============================================================
// when_all — concurrently await multiple tasks
// ============================================================

namespace detail {

/// Shared state for coordinating N concurrent tasks.
///
/// All state lives on the heap (shared_ptr) so that it outlives any
/// single coroutine frame involved in the coordination.
struct WhenAllState {
  std::exception_ptr exception;

  /// How many tasks have not yet completed.
  std::atomic<size_t> remaining;

  /// The coroutine_handle of the suspended when_all coroutine.
  ///
  /// Stored as an integer so we can use atomic memory ordering to
  /// avoid data races between the writer (WhenAllAwaiter::await_suspend)
  /// and the readers (the last launch wrapper that resumes when_all).
  std::atomic<uintptr_t> continuation{0};

  WhenAllState(size_t n) : remaining(n) {}

  /// Store the when_all continuation with release ordering.
  void set_continuation(std::coroutine_handle<> h) noexcept {
    continuation.store(reinterpret_cast<uintptr_t>(h.address()), std::memory_order_release);
  }

  /// Load the when_all continuation with acquire ordering.
  /// Returns nullptr if not yet set.
  std::coroutine_handle<> get_continuation() const noexcept {
    auto addr = continuation.load(std::memory_order_acquire);
    if (!addr)
      return nullptr;
    return std::coroutine_handle<>::from_address(reinterpret_cast<void*>(addr));
  }
};

/// Awaiter for when_all: suspends the caller until all tasks complete.
struct WhenAllAwaiter {
  std::shared_ptr<WhenAllState> state;

  bool await_ready() noexcept { return state->remaining.load(std::memory_order_acquire) == 0; }

  void await_suspend(std::coroutine_handle<> handle) { state->set_continuation(handle); }

  void await_resume() {
    if (state->exception)
      std::rethrow_exception(state->exception);
  }
};

/// Helper: launch a single task wrapper for when_all.
///
/// NOTE: This is a standalone function (not a lambda-in-loop) to work around
/// a GCC bug where coroutine lambdas capturing loop variables by value
/// may share the same capture across iterations.
///
/// IMPORTANT: The last wrapper to complete MUST resume the when_all
/// continuation on a **separate thread**.  If we called cont.resume()
/// directly from this coroutine body, when_all would run inline and its
/// eventual cleanup could destroy *this* coroutine's frame while it is
/// still on the call stack — a heap-use-after-free.
template <typename T>
Task<void> launch_wrapper(std::shared_ptr<WhenAllState> state, std::shared_ptr<std::vector<T>> results, size_t index,
                          Task<T> task) {
  try {
    (*results)[index] = co_await task;
  } catch (...) {
    state->exception = std::current_exception();
  }
  auto prev = state->remaining.fetch_sub(1, std::memory_order_acq_rel);
  if (prev == 1) {
    // All tasks done — resume the when_all continuation.
    // Spawn a new thread so that when_all's cleanup (which may destroy
    // THIS coroutine frame) cannot run while we are still on the stack.
    if (auto cont = state->get_continuation(); cont) {
      std::thread([cont] { cont.resume(); }).detach();
    }
  }
}

}  // namespace detail

/// Await multiple tasks of the same type concurrently.
///
/// Returns a Task that completes when all input tasks have completed.
/// Each task is started immediately (eagerly) and runs concurrently.
///
/// Note: True parallelism requires the individual tasks to suspend
/// for external events (e.g. io_uring). Tasks that run synchronously
/// with no suspension points will execute sequentially within a single
/// thread.
///
/// Example:
///   auto results = co_await when_all(std::vector<Task<int>>{...});
template <typename T>
Task<std::vector<T>> when_all(std::vector<Task<T>> tasks) {
  if (tasks.empty())
    co_return std::vector<T>{};

  auto state = std::make_shared<detail::WhenAllState>(tasks.size());
  auto results = std::make_shared<std::vector<T>>(tasks.size());

  // The wrappers stay alive on this coroutine's frame while suspended.
  std::vector<Task<void>> wrappers;
  wrappers.reserve(tasks.size());

  for (size_t i = 0; i < tasks.size(); ++i) {
    wrappers.push_back(detail::launch_wrapper<T>(state, results, i, std::move(tasks[i])));
  }

  // Start all wrappers. If a task completes synchronously, its
  // wrapper runs to completion inline within this loop.
  for (auto& w : wrappers) {
    w.handle().resume();
  }

  // Wait for all tasks to complete (or return immediately if all done).
  co_await detail::WhenAllAwaiter{state};
  co_return std::move(*results);
}

/// Await two tasks of potentially different types concurrently.
///
/// Example:
///   auto [a, b] = co_await when_all(task_a(), task_b());
template <typename T1, typename T2>
Task<std::tuple<T1, T2>> when_all(Task<T1> t1, Task<T2> t2) {
  struct SharedState {
    std::tuple<T1, T2> results;
    std::exception_ptr exception;
    std::shared_ptr<detail::WhenAllState> coord;
  };

  auto state = std::make_shared<SharedState>();
  state->coord = std::make_shared<detail::WhenAllState>(2);

  // Wrapper for the first task
  auto launch1 = [state](Task<T1> t) -> Task<void> {
    try {
      std::get<0>(state->results) = co_await t;
    } catch (...) {
      state->exception = std::current_exception();
    }
    if (state->coord->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      // Defer resumption to a new thread to avoid destroying our own frame.
      if (auto cont = state->coord->get_continuation(); cont) {
        std::thread([cont] { cont.resume(); }).detach();
      }
    }
  };

  // Wrapper for the second task
  auto launch2 = [state](Task<T2> t) -> Task<void> {
    try {
      std::get<1>(state->results) = co_await t;
    } catch (...) {
      state->exception = std::current_exception();
    }
    if (state->coord->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      if (auto cont = state->coord->get_continuation(); cont) {
        std::thread([cont] { cont.resume(); }).detach();
      }
    }
  };

  Task<void> w1 = launch1(std::move(t1));
  Task<void> w2 = launch2(std::move(t2));

  w1.handle().resume();
  w2.handle().resume();

  struct Awaiter {
    std::shared_ptr<SharedState> state;

    bool await_ready() noexcept { return state->coord->remaining.load(std::memory_order_acquire) == 0; }

    void await_suspend(std::coroutine_handle<> h) { state->coord->set_continuation(h); }

    void await_resume() {
      if (state->exception)
        std::rethrow_exception(state->exception);
    }
  };

  co_await Awaiter{state};
  co_return std::move(state->results);
}

}  // namespace sk::coro

#endif  // SK_UTILS_COROUTINE_TASK_H
