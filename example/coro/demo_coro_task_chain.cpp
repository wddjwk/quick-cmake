/// demo_coro_task_chain.cpp — Task Pipeline Composition & Parallel Execution
///
/// This example focuses on two key patterns for composing Tasks:
///
///   1. Pipeline (serial composition):
///        step_read() → step_parse() → step_fetch() → ...
///      Each step is a reusable Task that can be independently tested.
///
///   2. Parallel composition with when_all():
///        Run multiple independent Tasks concurrently and wait for all.
///
/// For the callback-hell comparison, see demo_coro_callback_hell.cpp.

#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "skutils/coroutine/coroutine.h"
#include "skutils/logger.h"

using namespace sk::coro;

// ============================================================
// Simulated async operations (callback-based, like legacy APIs)
// ============================================================

void async_read_file(const std::string& path, std::function<void(std::string)> callback) {
  std::thread([path, cb = std::move(callback)] {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    cb("data_from_" + path);
  }).detach();
}

void async_parse(const std::string& raw, std::function<void(std::string)> callback) {
  std::thread([raw, cb = std::move(callback)] {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    cb("parsed(" + raw + ")");
  }).detach();
}

void async_fetch(const std::string& key, std::function<void(std::string)> callback) {
  std::thread([key, cb = std::move(callback)] {
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    cb("info(" + key + ")");
  }).detach();
}

void async_write_file(const std::string& path, const std::string& content, std::function<void(bool)> callback) {
  std::thread([path, content, cb = std::move(callback)] {
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    cb(true);
  }).detach();
}

// ============================================================
// Awaitable wrappers — bridge callback APIs to coroutines
// ============================================================

struct ReadAwaiter {
  std::string path;
  std::string result;

  bool await_ready() { return false; }

  void await_suspend(std::coroutine_handle<> h) {
    async_read_file(path, [this, h](std::string d) {
      result = std::move(d);
      h.resume();
    });
  }

  std::string await_resume() { return std::move(result); }
};

struct ParseAwaiter {
  std::string raw;
  std::string result;

  bool await_ready() { return false; }

  void await_suspend(std::coroutine_handle<> h) {
    async_parse(raw, [this, h](std::string d) {
      result = std::move(d);
      h.resume();
    });
  }

  std::string await_resume() { return std::move(result); }
};

struct FetchAwaiter {
  std::string key;
  std::string result;

  bool await_ready() { return false; }

  void await_suspend(std::coroutine_handle<> h) {
    async_fetch(key, [this, h](std::string d) {
      result = std::move(d);
      h.resume();
    });
  }

  std::string await_resume() { return std::move(result); }
};

struct WriteAwaiter {
  std::string path;
  std::string content;
  bool success = false;

  bool await_ready() { return false; }

  void await_suspend(std::coroutine_handle<> h) {
    async_write_file(path, content, [this, h](bool ok) {
      success = ok;
      h.resume();
    });
  }

  bool await_resume() { return success; }
};

// ============================================================
// Part 1: Task Pipeline — each step is a reusable Task
// ============================================================

/// Each function below is a self-contained async Task.
/// They can be tested independently, composed, or reused.

Task<std::string> step_read(const std::string& path) {
  auto data = co_await ReadAwaiter{path};
  co_return data;
}

Task<std::string> step_parse(const std::string& raw) {
  auto parsed = co_await ParseAwaiter{raw};
  co_return parsed;
}

Task<std::string> step_fetch(const std::string& key) {
  auto info = co_await FetchAwaiter{key};
  co_return info;
}

Task<bool> step_write(const std::string& path, const std::string& content) {
  auto ok = co_await WriteAwaiter{path, content};
  co_return ok;
}

/// Pipeline: compose steps sequentially using co_await.
/// Each step is a separate Task — the pipeline suspends between steps.
Task<std::string> pipeline(const std::string& filename) {
  SK_LOG("  [pipeline] start");

  auto raw = co_await step_read(filename);
  SK_LOG("  [pipeline] read: {}", raw);

  auto parsed = co_await step_parse(raw);
  SK_LOG("  [pipeline] parsed: {}", parsed);

  auto info = co_await step_fetch("lookup_key");
  SK_LOG("  [pipeline] fetched: {}", info);

  auto result = parsed + " + " + info;

  auto ok = co_await step_write("output.dat", result);
  SK_LOG("  [pipeline] write success: {}", ok);

  co_return result;
}

void demo_pipeline() {
  SK_LOG("=== Task Pipeline ===");
  auto result = sync_await(pipeline("pipeline.dat"));
  SK_LOG("pipeline result: {}", result);
}

// ============================================================
// Part 2: Parallel Execution with when_all
// ============================================================

/// Simulate an async fetch that takes variable time.
Task<std::string> fetch_data(std::string name, int delay_ms) {
  SK_LOG("  [fetch] {} starting ({}ms)...", name, delay_ms);
  // Simulate actual async work
  std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
  SK_LOG("  [fetch] {} done", name);
  co_return "data[" + name + "]";
}

/// Run multiple fetches concurrently and wait for all.
/// Total time ≈ max(delay_ms), not sum(delay_ms).
Task<void> demo_when_all_vector() {
  SK_LOG("=== when_all (std::vector) ===");

  auto start = std::chrono::steady_clock::now();

  // Launch all tasks, gather results when all complete
  std::vector<Task<std::string>> tasks;
  tasks.push_back(fetch_data("A", 100));
  tasks.push_back(fetch_data("B", 150));
  tasks.push_back(fetch_data("C", 80));

  auto results = co_await when_all(std::move(tasks));

  auto elapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

  for (size_t i = 0; i < results.size(); ++i) {
    SK_LOG("  Result[{}]: {}", i, results[i]);
  }
  SK_LOG("  Total time: {}ms (sequential would be ~330ms)", elapsed);
}

/// when_all also works with two heterogeneous task types.
Task<void> demo_when_all_pair() {
  SK_LOG("=== when_all (pair) ===");

  auto [name, age] = co_await when_all(fetch_data("get_name", 50), fetch_data("get_age", 100));
  SK_LOG("  Name: {}", name);
  SK_LOG("  Age: {}", age);
}

// ============================================================
// Part 3: Pipeline + Concurrency (read in parallel, then process)
// ============================================================

/// A more realistic scenario: read two files in parallel,
/// then combine and process the results.
Task<void> demo_parallel_then_sequential() {
  SK_LOG("=== Parallel Read → Sequential Process ===");

  auto start = std::chrono::steady_clock::now();

  // Read two files concurrently
  auto [content_a, content_b] = co_await when_all(step_read("config_a.txt"), step_read("config_b.txt"));

  SK_LOG("  Read A: {}", content_a);
  SK_LOG("  Read B: {}", content_b);

  // Process the combined result
  auto combined = co_await step_parse(content_a + " | " + content_b);
  SK_LOG("  Combined: {}", combined);

  auto elapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
  SK_LOG("  Total time: {}ms (reads overlapped)", elapsed);
}

// ============================================================
// Main
// ============================================================

int main() {
  demo_pipeline();
  std::cout << "\n";

  sync_await(demo_when_all_vector());
  std::cout << "\n";

  sync_await(demo_when_all_pair());
  std::cout << "\n";

  sync_await(demo_parallel_then_sequential());

  return 0;
}
