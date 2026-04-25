/// demo_coro_callback_hell.cpp — Callback Hell vs Coroutine Style
///
/// The single most compelling reason for coroutines: converting deeply
/// nested callbacks into flat, sequential, readable code.
///
/// Compare these two implementations of the same async workflow:
///   callback style  →  deeply nested "pyramid of doom"
///   coroutine style →  flat, sequential, looks like sync code

#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

#include "skutils/coroutine/coroutine.h"
#include "skutils/logger.h"

using namespace sk::coro;

// ============================================================
// Simulated async operations (callback-based, like legacy APIs)
// ============================================================

using Callback = std::function<void(std::string)>;

/// Simulate an async DB query.
void db_query(const std::string& sql, Callback on_success, Callback on_error) {
  std::thread([sql, on_success = std::move(on_success), on_error = std::move(on_error)] {
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    if (sql.find("INVALID") != std::string::npos)
      on_error("SQL error: invalid query");
    else
      on_success("result: " + sql);
  }).detach();
}

/// Simulate an async cache lookup.
void cache_get(const std::string& key, Callback on_hit, Callback on_miss) {
  std::thread([key, on_hit = std::move(on_hit), on_miss = std::move(on_miss)] {
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    on_miss("miss:" + key);  // Always miss in this simulation
  }).detach();
}

/// Simulate an async HTTP fetch.
void http_fetch(const std::string& url, Callback on_success, Callback on_error) {
  std::thread([url, on_success = std::move(on_success), on_error = std::move(on_error)] {
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    on_success("response: " + url);
  }).detach();
}

// ============================================================
// Style 1: Callback Hell (Pyramid of Doom)
// ============================================================

void handle_request_callback() {
  SK_LOG("--- CALLBACK STYLE ---");

  cache_get(
    "user:123",
    /* hit */ [](std::string cached) { SK_LOG("  cache hit: {}", cached); },
    /* miss */
    [](std::string) {
      db_query(
        "SELECT * FROM users WHERE id=123",
        /* success */
        [](std::string db_result) {
          http_fetch(
            "https://api.example.com/users/123",
            /* success */
            [db_result](std::string api_response) {
              SK_LOG("  response: {} | {}", db_result, api_response);
              SK_LOG("  Done (callback style)!");
            },
            /* error */ [](std::string err) { SK_LOG("  http error: {}", err); });
        },
        /* error */ [](std::string err) { SK_LOG("  db error: {}", err); });
    });

  std::this_thread::sleep_for(std::chrono::milliseconds(400));
}

// ============================================================
// Style 2: Coroutine Style (Flat, Sequential)
// ============================================================

// -- Awaitable wrappers that bridge callbacks to coroutines --

struct DbQueryAwaiter {
  std::string sql;
  std::string result;
  std::string error;
  bool ok = true;

  bool await_ready() { return false; }

  void await_suspend(std::coroutine_handle<> handle) {
    db_query(
      sql,
      [this, handle](std::string res) {
        result = std::move(res);
        handle.resume();
      },
      [this, handle](std::string err) {
        error = std::move(err);
        ok = false;
        handle.resume();
      });
  }

  std::string await_resume() {
    if (!ok)
      throw std::runtime_error(error);
    return std::move(result);
  }
};

struct CacheGetAwaiter {
  std::string key;
  std::string result;
  bool hit = false;

  bool await_ready() { return false; }

  void await_suspend(std::coroutine_handle<> handle) {
    cache_get(
      key,
      [this, handle](std::string v) {
        result = std::move(v);
        hit = true;
        handle.resume();
      },
      [this, handle](std::string v) {
        result = std::move(v);
        hit = false;
        handle.resume();
      });
  }

  std::string await_resume() { return std::move(result); }
};

struct HttpFetchAwaiter {
  std::string url;
  std::string result;
  std::string error;
  bool ok = true;

  bool await_ready() { return false; }

  void await_suspend(std::coroutine_handle<> handle) {
    http_fetch(
      url,
      [this, handle](std::string res) {
        result = std::move(res);
        handle.resume();
      },
      [this, handle](std::string err) {
        error = std::move(err);
        ok = false;
        handle.resume();
      });
  }

  std::string await_resume() {
    if (!ok)
      throw std::runtime_error(error);
    return std::move(result);
  }
};

/// The coroutine version — reads like synchronous code!
/// Compare this flat, linear flow to the nested callbacks above.
Task<void> handle_request_coroutine() {
  SK_LOG("--- COROUTINE STYLE ---");

  try {
    auto cached = co_await CacheGetAwaiter{"user:123"};
    SK_LOG("  cache: {}", cached);

    auto db_result = co_await DbQueryAwaiter{"SELECT * FROM users WHERE id=123"};
    auto api_response = co_await HttpFetchAwaiter{"https://api.example.com/users/123"};

    SK_LOG("  response: {} | {}", db_result, api_response);
    SK_LOG("  Done (coroutine style)!");
  } catch (const std::exception& e) {
    // One single catch handles errors from any async step —
    // no error callbacks scattered across nested scopes!
    SK_LOG("  Error: {}", e.what());
  }
}

// ============================================================
// Error handling comparison
// ============================================================

Task<void> handle_with_error() {
  SK_LOG("--- COROUTINE ERROR HANDLING ---");
  try {
    (void)co_await DbQueryAwaiter{"INVALID SQL"};
  } catch (const std::exception& e) {
    SK_LOG("  Caught: {}", e.what());
  }
}

// ============================================================
// Main
// ============================================================

int main() {
  SK_LOG("=== Callback Hell vs Coroutine ===");
  std::cout << "\n";

  handle_request_callback();
  std::cout << "\n";

  sync_await(handle_request_coroutine());
  std::cout << "\n";

  sync_await(handle_with_error());

  return 0;
}
