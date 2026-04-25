/// demo_coro_concurrencpp.cpp — concurrencpp comparison
///
/// concurrencpp is a mature C++20 coroutine library that provides:
///   - result<T>       (similar to our Task<T>)
///   - lazy_result<T>  (deferred execution)
///   - coroutine-based parallelism (when_all, when_any)
///   - Built-in thread pool, timer, and executor infrastructure
///
/// This example demonstrates concurrencpp's approach and compares
/// it with our minimal sk::coro library.

#include <concurrencpp/concurrencpp.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "skutils/coroutine/coroutine.h"
#include "skutils/logger.h"

// ============================================================
// Part 1: Basic result<T> — concurrencpp's equivalent of Task<T>
// ============================================================

/// concurrencpp::result<T> is the return type of a concurrencpp coroutine.
/// Unlike our Task<T>, it is tied to concurrencpp's runtime (thread pool).
/// You must pass an executor to resume_on() to schedule work on a thread.
concurrencpp::result<int> concurrencpp_compute(std::shared_ptr<concurrencpp::thread_pool_executor> tpe, int x) {
  co_await concurrencpp::resume_on(tpe);  // resume on a worker thread
  SK_LOG("  [concurrencpp] computing {}^2...", x);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  co_return x* x;
}

concurrencpp::result<std::string> concurrencpp_greet(std::shared_ptr<concurrencpp::thread_pool_executor> tpe,
                                                     const std::string& name) {
  co_await concurrencpp::resume_on(tpe);
  co_return "Hello, " + name + " from concurrencpp!";
}

void demo_concurrencpp_basics() {
  SK_LOG("=== concurrencpp Basics ===");

  // concurrencpp requires a runtime object
  concurrencpp::runtime runtime;

  auto tpe = runtime.thread_pool_executor();

  // Get results — .get() blocks like sync_await
  auto result = concurrencpp_compute(tpe, 7).get();
  SK_LOG("  7^2 = {}", result);

  auto greeting = concurrencpp_greet(tpe, "World").get();
  SK_LOG("  {}", greeting);
}

// ============================================================
// Part 2: Task composition — compare with sk::coro
// ============================================================

concurrencpp::result<int> cc_add(std::shared_ptr<concurrencpp::thread_pool_executor> tpe, int a, int b) {
  co_await concurrencpp::resume_on(tpe);
  co_return a + b;
}

concurrencpp::result<int> cc_multiply(std::shared_ptr<concurrencpp::thread_pool_executor> tpe, int a, int b) {
  co_await concurrencpp::resume_on(tpe);
  co_return a* b;
}

/// Compose — just like our Task composition
concurrencpp::result<int> cc_compute(std::shared_ptr<concurrencpp::thread_pool_executor> tpe) {
  auto sum = co_await cc_add(tpe, 3, 4);
  SK_LOG("  [concurrencpp] sum = {}", sum);
  auto product = co_await cc_multiply(tpe, sum, 2);
  SK_LOG("  [concurrencpp] product = {}", product);
  co_return product;
}

/// Same logic with sk::coro
sk::coro::Task<int> sk_add(int a, int b) {
  co_return a + b;
}

sk::coro::Task<int> sk_multiply(int a, int b) {
  co_return a* b;
}

sk::coro::Task<int> sk_compute() {
  auto sum = co_await sk_add(3, 4);
  SK_LOG("  [sk::coro] sum = {}", sum);
  auto product = co_await sk_multiply(sum, 2);
  SK_LOG("  [sk::coro] product = {}", product);
  co_return product;
}

void demo_composition_comparison() {
  SK_LOG("=== Composition Comparison ===");

  // concurrencpp
  {
    concurrencpp::runtime runtime;
    auto tpe = runtime.thread_pool_executor();
    auto cc_result = cc_compute(tpe).get();
    SK_LOG("  concurrencpp result: {}", cc_result);
  }

  // sk::coro
  {
    auto sk_result = sk::coro::sync_await(sk_compute());
    SK_LOG("  sk::coro result: {}", sk_result);
  }
}

// ============================================================
// Part 3: when_all — parallel composition
// ============================================================

/// concurrencpp provides when_all to run tasks in parallel.
/// This is something our minimal sk::coro doesn't provide out of the box.
concurrencpp::result<int> cc_slow_task(std::shared_ptr<concurrencpp::thread_pool_executor> tpe, int value,
                                       int delay_ms) {
  co_await concurrencpp::resume_on(tpe);
  std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
  SK_LOG("  [concurrencpp] slow_task({}) done after {}ms", value, delay_ms);
  co_return value;
}

void demo_when_all() {
  SK_LOG("=== concurrencpp when_all (parallel) ===");

  concurrencpp::runtime runtime;
  auto tpe = runtime.thread_pool_executor();

  auto start = std::chrono::steady_clock::now();

  // Run three tasks in parallel — when_all returns when ALL complete
  // when_all requires a resume executor (shared_ptr)
  auto results =
    concurrencpp::when_all(tpe, cc_slow_task(tpe, 1, 100), cc_slow_task(tpe, 2, 150), cc_slow_task(tpe, 3, 80));

  auto result_vec = results.run().get();
  auto end = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  int total = 0;
  total += std::get<0>(result_vec).get();
  total += std::get<1>(result_vec).get();
  total += std::get<2>(result_vec).get();

  SK_LOG("  Total: {}, elapsed: {}ms (parallel, should be ~150ms not 330ms)", total, elapsed);
}

// ============================================================
// Part 4: Comparison table
// ============================================================

void print_comparison() {
  SK_LOG("\n=== Comparison: sk::coro vs concurrencpp ===");
  std::cout << "\n";
  std::cout << "Feature               | sk::coro              | concurrencpp\n";
  std::cout << "----------------------|-----------------------|--------------------------\n";
  std::cout << "Coroutine type        | Task<T>               | result<T>\n";
  std::cout << "Lazy type             | Task<T> (always lazy) | lazy_result<T>\n";
  std::cout << "Generator             | Generator<T>         | Not built-in\n";
  std::cout << "Sync await            | sync_await()         | .get()\n";
  std::cout << "Parallel (when_all)   | Not built-in         | Built-in\n";
  std::cout << "Race (when_any)       | Not built-in         | Built-in\n";
  std::cout << "Thread pool           | Not built-in         | Built-in runtime\n";
  std::cout << "Timer support         | Not built-in         | Built-in timer_queue\n";
  std::cout << "Executor abstraction  | None                 | Multiple executors\n";
  std::cout << "Dependencies          | None (std only)      | concurrencpp runtime\n";
  std::cout << "C++ standard          | C++20                | C++20\n";
  std::cout << "Size / complexity     | ~300 LOC             | Full framework\n";
  std::cout << "\n";
  std::cout << "sk::coro is a minimal library that shows the C++20 coroutine\n";
  std::cout << "mechanism. concurrencpp is a production framework with\n";
  std::cout << "runtime, thread pools, and composition utilities.\n";
}

// ============================================================
// Main
// ============================================================

int main() {
  demo_concurrencpp_basics();
  std::cout << "\n";
  demo_composition_comparison();
  std::cout << "\n";
  demo_when_all();
  print_comparison();
  return 0;
}
