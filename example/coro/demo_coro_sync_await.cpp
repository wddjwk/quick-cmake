/// demo_coro_sync_await.cpp — sync_await() in depth
///
/// sync_await() is a bridge between the coroutine world and the
/// synchronous world. It blocks the calling thread until the task
/// completes and returns the result.
///
/// This is particularly useful in main() or test code where you
/// need a concrete value from an async computation.

#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "skutils/coroutine/coroutine.h"
#include "skutils/logger.h"

using namespace sk::coro;

// ============================================================
// Basic sync_await usage
// ============================================================

Task<int> async_compute(int x) {
  SK_LOG("  computing square of {}...", x);
  // Simulate async work
  std::async(std::launch::async, [] { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }).wait();
  co_return x* x;
}

Task<std::string> async_greet(const std::string& name) {
  co_return "Hello, " + name + "!";
}

Task<void> async_log(const std::string& msg) {
  SK_LOG("  [async_log] {}", msg);
  co_return;
}

void demo_basic() {
  SK_LOG("=== Basic sync_await ===");

  // Blocking wait for a Task<int>
  int result = sync_await(async_compute(7));
  SK_LOG("7^2 = {}", result);

  // Blocking wait for a Task<string>
  auto greeting = sync_await(async_greet("Coroutine"));
  SK_LOG("greeting: {}", greeting);

  // Blocking wait for a Task<void>
  sync_await(async_log("This is an async log message"));
}

// ============================================================
// Chaining with sync_await
// ============================================================

/// Each step is a separate Task — can be composed or used standalone.
Task<int> fetch_user_id(const std::string& username) {
  SK_LOG("  fetch_user_id: {}", username);
  co_return 42;
}

Task<std::string> fetch_user_name(int id) {
  SK_LOG("  fetch_user_name: id={}", id);
  co_return "Alice";
}

Task<int> fetch_user_age(const std::string& name) {
  SK_LOG("  fetch_user_age: name={}", name);
  co_return 30;
}

void demo_chaining() {
  SK_LOG("=== Chaining with sync_await ===");

  // You can chain sync_await calls, but this blocks the thread
  // between each step. For true async composition, use co_await
  // inside another Task instead (see demo_coro_task_chain.cpp).
  auto id = sync_await(fetch_user_id("alice123"));
  auto name = sync_await(fetch_user_name(id));
  auto age = sync_await(fetch_user_age(name));

  SK_LOG("User: id={}, name={}, age={}", id, name, age);
}

// ============================================================
// sync_await with Task composition (the right way)
// ============================================================

/// The CORRECT way to compose async operations: use co_await inside
/// a Task, and only call sync_await once at the top level.
Task<std::string> fetch_user_info(const std::string& username) {
  auto id = co_await fetch_user_id(username);
  auto name = co_await fetch_user_name(id);
  auto age = co_await fetch_user_age(name);
  co_return std::format("{} (id={}, age={})", name, id, age);
}

void demo_composition() {
  SK_LOG("=== Proper Composition (one sync_await at top) ===");

  // Only one blocking call — everything inside is async
  auto info = sync_await(fetch_user_info("alice123"));
  SK_LOG("User info: {}", info);
}

// ============================================================
// Parallel execution with sync_await
// ============================================================

Task<int> slow_add(int a, int b) {
  SK_LOG("  slow_add: {} + {}", a, b);
  std::async(std::launch::async, [] { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }).wait();
  co_return a + b;
}

Task<int> slow_multiply(int a, int b) {
  SK_LOG("  slow_multiply: {} * {}", a, b);
  std::async(std::launch::async, [] { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }).wait();
  co_return a* b;
}

void demo_parallel() {
  SK_LOG("=== Parallel with sync_await ===");

  // NOTE: sync_await is inherently sequential — it blocks until
  // each task completes. For parallel execution, you'd need an
  // event loop or WhenAll utility. Here we show the sequential case:

  auto sum = sync_await(slow_add(3, 4));
  auto product = sync_await(slow_multiply(sum, 2));

  SK_LOG("(3+4)*2 = {}", product);
}

// ============================================================
// Main
// ============================================================

int main() {
  demo_basic();
  std::cout << "\n";
  demo_chaining();
  std::cout << "\n";
  demo_composition();
  std::cout << "\n";
  demo_parallel();
  return 0;
}
