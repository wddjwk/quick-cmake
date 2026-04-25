/// demo_coro_asio.cpp — Asio coroutine examples for comparison
///
/// Asio (standalone) provides C++20 native coroutine support via:
///   1. asio::awaitable<T> + co_await + co_return
///   2. asio::use_awaitable completion token
///   3. asio::co_spawn to launch awaitables on io_context
///
/// This example compares asio's approach with our sk::coro approach.
///
/// Key differences:
///   - asio::awaitable is tightly coupled with asio::io_context (their event loop)
///   - sk::coro::Task is event-loop-agnostic — you choose the executor
///   - asio provides a rich set of built-in async operations
///   - sk::coro is minimal and focused on the coroutine primitives only

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "skutils/logger.h"

// Asio standalone mode
#define ASIO_STANDALONE
#include <asio.hpp>

#include "skutils/coroutine/coroutine.h"

// ============================================================
// Part 1: asio::awaitable<T> — asio's native coroutine type
// ============================================================

/// asio's awaitable<T> is similar to our Task<T> but requires
/// running within an asio::io_context. The co_await operations
/// are implicitly scheduled on the io_context's executor.
asio::awaitable<void> asio_timer_example() {
  auto executor = co_await asio::this_coro::executor;

  SK_LOG("  [asio] Starting timer...");
  asio::steady_timer timer(executor, std::chrono::milliseconds(200));
  co_await timer.async_wait(asio::use_awaitable);
  SK_LOG("  [asio] Timer expired!");
}

/// Async computation returning a value
asio::awaitable<std::string> asio_async_read_file() {
  auto executor = co_await asio::this_coro::executor;
  asio::steady_timer timer(executor, std::chrono::milliseconds(100));
  co_await timer.async_wait(asio::use_awaitable);
  co_return "file_content_from_asio";
}

/// Compose asio awaitables — similar to our Task composition
asio::awaitable<std::string> asio_compose() {
  auto content = co_await asio_async_read_file();
  SK_LOG("  [asio] Got content: {}", content);

  auto executor = co_await asio::this_coro::executor;
  asio::steady_timer timer(executor, std::chrono::milliseconds(50));
  co_await timer.async_wait(asio::use_awaitable);

  co_return "processed(" + content + ")";
}

/// Print the result of a composition
asio::awaitable<void> asio_print_compose_result() {
  auto result = co_await asio_compose();
  SK_LOG("  [asio] Compose result: {}", result);
}

void demo_asio_awaitable() {
  SK_LOG("=== asio::awaitable<T> Demo ===");

  asio::io_context ioc;

  // Launch the timer coroutine
  asio::co_spawn(ioc, asio_timer_example(), asio::detached);
  ioc.run();
  ioc.restart();

  // Launch the composed coroutine
  asio::co_spawn(ioc, asio_print_compose_result(), asio::detached);
  ioc.run();
}

// ============================================================
// Part 2: asio TCP echo — real network example
// ============================================================

asio::awaitable<void> asio_echo_server(asio::ip::tcp::endpoint endpoint) {
  auto executor = co_await asio::this_coro::executor;
  asio::ip::tcp::acceptor acceptor(executor, endpoint);

  SK_LOG("  [asio] Echo server listening on port {}", endpoint.port());

  // Accept one connection
  auto socket = co_await acceptor.async_accept(asio::use_awaitable);
  SK_LOG("  [asio] Client connected!");

  // Echo loop
  char data[1024];
  try {
    while (true) {
      std::size_t n = co_await socket.async_read_some(asio::buffer(data), asio::use_awaitable);
      co_await asio::async_write(socket, asio::buffer(data, n), asio::use_awaitable);
      SK_LOG("  [asio] Echoed {} bytes", n);
    }
  } catch (const asio::system_error& e) {
    if (e.code() != asio::error::eof) {
      SK_LOG("  [asio] Error: {}", e.what());
    }
  }
}

asio::awaitable<void> asio_echo_client(asio::ip::tcp::endpoint endpoint) {
  auto executor = co_await asio::this_coro::executor;

  // Small delay to let server start
  asio::steady_timer timer(executor, std::chrono::milliseconds(50));
  co_await timer.async_wait(asio::use_awaitable);

  asio::ip::tcp::socket socket(executor);
  co_await socket.async_connect(endpoint, asio::use_awaitable);

  // Send a message
  const std::string msg = "Hello from asio coroutine!";
  co_await asio::async_write(socket, asio::buffer(msg), asio::use_awaitable);
  SK_LOG("  [asio/client] Sent: {}", msg);

  // Read echo
  char reply[256];
  auto n = co_await socket.async_read_some(asio::buffer(reply), asio::use_awaitable);
  SK_LOG("  [asio/client] Received: {}", std::string(reply, n));

  // Close
  socket.close();
}

void demo_asio_network() {
  SK_LOG("=== asio Network (TCP Echo) Demo ===");

  asio::io_context ioc;
  auto endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0);

  // Start server
  asio::ip::tcp::acceptor acceptor(ioc.get_executor(), endpoint);
  auto server_endpoint = acceptor.local_endpoint();
  acceptor.close();  // Let the coroutine acceptor bind

  asio::co_spawn(ioc, asio_echo_server(server_endpoint), asio::detached);
  asio::co_spawn(ioc, asio_echo_client(server_endpoint), asio::detached);

  ioc.run();
}

// ============================================================
// Part 3: Comparison with sk::coro — same timer pattern
// ============================================================

/// The same timer pattern implemented with sk::coro.
/// Compare how much simpler it is when you don't need io_context.
sk::coro::Task<void> sk_timer_example() {
  SK_LOG("  [sk::coro] Starting timer (simulated)...");
  // sk::coro doesn't have a built-in timer, so we simulate it.
  // In a real app, you'd integrate with an event loop or io_uring.
  std::async(std::launch::async, [] { std::this_thread::sleep_for(std::chrono::milliseconds(200)); }).wait();
  SK_LOG("  [sk::coro] Timer expired!");
  co_return;
}

sk::coro::Task<std::string> sk_async_read() {
  std::async(std::launch::async, [] { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }).wait();
  co_return "file_content_from_sk_coro";
}

sk::coro::Task<std::string> sk_compose() {
  auto content = co_await sk_async_read();
  SK_LOG("  [sk::coro] Got content: {}", content);
  co_return "processed(" + content + ")";
}

void demo_sk_coro_comparison() {
  SK_LOG("=== sk::coro Equivalent Demo ===");

  // No io_context needed — just sync_await
  sk::coro::sync_await(sk_timer_example());
  auto result = sk::coro::sync_await(sk_compose());
  SK_LOG("  [sk::coro] Compose result: {}", result);
}

// ============================================================
// Part 4: Comparison table
// ============================================================

void print_comparison() {
  SK_LOG("\n=== Comparison: sk::coro vs asio ===");
  std::cout << "\n";
  std::cout << "Feature               | sk::coro                    | asio\n";
  std::cout << "----------------------|-----------------------------|----------------------\n";
  std::cout << "Coroutine type        | Task<T>                     | awaitable<T>\n";
  std::cout << "Generator             | Generator<T> (co_yield)    | Not built-in\n";
  std::cout << "Event loop            | EventLoop (simple)          | io_context (rich)\n";
  std::cout << "Async ops             | Bring your own / io_uring   | Built-in timers, net\n";
  std::cout << "Executor coupling     | Decoupled                   | Coupled to io_context\n";
  std::cout << "Sync await            | sync_await()                | Not built-in\n";
  std::cout << "Task composition      | co_await inside Task        | co_await inside awaitable\n";
  std::cout << "Error handling        | Exceptions (like sync code) | error_code or except\n";
  std::cout << "Header-only           | Yes                         | Yes (standalone)\n";
  std::cout << "Dependencies          | None (std lib only)         | asio + Threads\n";
  std::cout << "\n";
  std::cout << "sk::coro is a minimal teaching/reference library.\n";
  std::cout << "asio is a production-grade async framework.\n";
  std::cout << "For real projects, use asio (or Boost.Asio) with its native\n";
  std::cout << "awaitable support. sk::coro helps you understand the primitives.\n";
}

// ============================================================
// Main
// ============================================================

int main() {
  demo_asio_awaitable();
  std::cout << "\n";
  demo_asio_network();
  std::cout << "\n";
  demo_sk_coro_comparison();
  print_comparison();
  return 0;
}
