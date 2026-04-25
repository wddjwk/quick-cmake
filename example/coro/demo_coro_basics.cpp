/// demo_coro_basics.cpp — Generator<T> and Task<T> fundamentals
///
/// This example walks through the two core coroutine types in sk::coro:
///
///   1. Generator<T>  — lazy sequence producer (co_yield)
///   2. Task<T>       — async computation (co_await / co_return)
///
/// Each section explains the C++20 coroutine mechanism being used.

#include <future>
#include <iostream>
#include <string>
#include <vector>

#include "skutils/coroutine/coroutine.h"
#include "skutils/logger.h"

using namespace sk::coro;

// ============================================================
// Part 1: Generator<T> — Lazy sequences with co_yield
// ============================================================

/// A simple generator that yields integers from 0 to n-1.
///
/// How it works:
///   - The function signature returns Generator<int>, which tells the
///     compiler this is a coroutine.
///   - Each co_yield suspends the coroutine and produces a value.
///   - The coroutine is lazy: it only executes when the consumer
///     (the iterator) requests the next value.
///   - The coroutine frame (local variables, suspension points) is
///     heap-allocated and lives until the Generator object is destroyed.
Generator<int> range(int n) {
  for (int i = 0; i < n; ++i) {
    co_yield i;  // Suspend here, yield i to the consumer
  }
  // co_return is implicit (returns void) when the function ends
}

/// Fibonacci sequence generator — infinite (bounded by consumer).
///
/// Demonstrates that local variables (a, b) persist across
/// suspension points — this is the key property of coroutines.
Generator<int> fibonacci(int count) {
  int a = 0, b = 1;
  for (int i = 0; i < count; ++i) {
    co_yield a;
    int tmp = a;
    a = b;
    b = tmp + b;
  }
}

/// Generator with string values.
Generator<std::string> words() {
  co_yield "hello";
  co_yield "world";
  co_yield "coroutine";
}

void demo_generator() {
  SK_LOG("=== Generator<T> Demo ===");

  // Generator works with range-for because it provides begin()/end()
  SK_LOG("range(5):");
  for (auto v : range(5)) {
    std::cout << v << " ";
  }
  std::cout << "\n";

  SK_LOG("fibonacci(10):");
  for (auto v : fibonacci(10)) {
    std::cout << v << " ";
  }
  std::cout << "\n";

  SK_LOG("words():");
  for (const auto& w : words()) {
    std::cout << "[" << w << "] ";
  }
  std::cout << "\n";
}

// ============================================================
// Part 2: Task<T> — Asynchronous computations with co_await
// ============================================================

/// The simplest Task: just return a value.
///
/// Key points about Task<T>:
///   - Task<T> is LAZY — it does not start executing until someone
///     co_awaits it (or you call sync_await).
///   - co_return stores the result in the promise object.
///   - The awaiter machinery handles suspending the caller and
///     resuming it when the task completes.
Task<int> simple_task() {
  SK_LOG("  simple_task: computing 42...");
  co_return 42;
}

/// Task composition: co_await one task from inside another.
///
/// This is where coroutines truly shine — you can compose async
/// operations sequentially, as if they were synchronous calls.
Task<int> add_task(int a, int b) {
  SK_LOG("  add_task: {} + {}", a, b);
  co_return a + b;
}

Task<int> multiply_task(int a, int b) {
  SK_LOG("  multiply_task: {} * {}", a, b);
  co_return a* b;
}

/// Compose: (a + b) * c — looks like synchronous code but is async!
Task<int> compute_task(int a, int b, int c) {
  SK_LOG("  compute_task: start");
  auto sum = co_await add_task(a, b);  // Suspend until add_task completes
  SK_LOG("  compute_task: got sum = {}", sum);
  auto result = co_await multiply_task(sum, c);  // Then multiply
  SK_LOG("  compute_task: got result = {}", result);
  co_return result;
}

/// Task<void> — coroutine that doesn't return a value.
Task<void> log_task(const std::string& msg) {
  SK_LOG("  log_task: {}", msg);
  co_return;  // or just let the function end
}

void demo_task() {
  SK_LOG("=== Task<T> Demo ===");

  // sync_await blocks the current thread until the task completes.
  // For simple tasks this is straightforward.
  SK_LOG("simple_task result: {}", sync_await(simple_task()));

  // Task composition — notice the code reads like synchronous computation
  SK_LOG("compute_task(2, 3, 4) = (2+3)*4 = {}", sync_await(compute_task(2, 3, 4)));

  // Task<void>
  sync_await(log_task("Hello from Task<void>!"));
}

// ============================================================
// Part 3: Understanding the Coroutine Framework
// ============================================================

/// Custom Awaitable — demonstrates how co_await works under the hood.
///
/// When you write `auto x = co_await expr;`, the compiler:
///   1. Calls expr.await_ready() — if true, don't suspend at all
///   2. Calls expr.await_suspend(handle) — if returns void or true,
///      the coroutine is suspended; the handle is the current coroutine
///   3. When resumed, calls expr.await_resume() — its return value
///      becomes the result of co_await
struct DelayAwaiter {
  int milliseconds;

  /// If true, skip suspension entirely (optimization for ready results)
  bool await_ready() { return false; }

  /// Called when the coroutine is about to suspend.
  /// We simulate an async delay using std::async, then resume.
  void await_suspend(std::coroutine_handle<> handle) {
    // In a real async system, you'd register with an event loop.
    // Here we simulate: start a timer, resume when it fires.
    std::async(std::launch::async, [handle, ms = milliseconds] {
      std::this_thread::sleep_for(std::chrono::milliseconds(ms));
      handle.resume();  // Resume the suspended coroutine
    });
  }

  /// The result of co_await — here we just return the delay value
  int await_resume() { return milliseconds; }
};

Task<int> delayed_value(int value, int delay_ms) {
  SK_LOG("  delayed_value: waiting {} ms...", delay_ms);
  auto waited = co_await DelayAwaiter{delay_ms};
  SK_LOG("  delayed_value: waited {} ms, returning {}", waited, value);
  co_return value;
}

void demo_custom_awaitable() {
  SK_LOG("=== Custom Awaitable Demo ===");
  SK_LOG("delayed_value(99, 100) = {}", sync_await(delayed_value(99, 100)));
}

// ============================================================
// Part 4: Exception Propagation
// ============================================================

Task<int> throwing_task() {
  co_await DelayAwaiter{10};
  throw std::runtime_error("Something went wrong!");
  co_return 0;  // unreachable
}

Task<int> catching_task() {
  try {
    auto val = co_await throwing_task();
    co_return val;
  } catch (const std::runtime_error& e) {
    SK_LOG("  caught exception: {}", e.what());
    co_return -1;
  }
}

void demo_exceptions() {
  SK_LOG("=== Exception Propagation Demo ===");
  SK_LOG("catching_task result: {}", sync_await(catching_task()));
}

// ============================================================
// Main
// ============================================================

int main() {
  demo_generator();
  std::cout << "\n";
  demo_task();
  std::cout << "\n";
  demo_custom_awaitable();
  std::cout << "\n";
  demo_exceptions();
  return 0;
}
