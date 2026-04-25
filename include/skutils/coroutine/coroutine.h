#ifndef SK_UTILS_COROUTINE_COROUTINE_H
#define SK_UTILS_COROUTINE_COROUTINE_H

/// sk::coro — A lightweight C++20 coroutine library.
///
/// Components:
///   Generator<T>   — lazy sequence via co_yield  (include/skutils/coroutine/generator.h)
///   Task<T>        — async computation via co_await / co_return  (include/skutils/coroutine/task.h)
///   when_all()     — concurrently await multiple Tasks  (include/skutils/coroutine/task.h)
///   sync_await()   — blocking wait for a Task  (include/skutils/coroutine/sync_await.h)
///   EventLoop      — simple event loop for scheduling  (include/skutils/coroutine/event_loop.h)
///   io_uring       — Linux io_uring async IO (only with SK_HAS_IO_URING)  (include/skutils/coroutine/io_uring_async.h)
///
/// Quick start:
///
///   #include <skutils/coroutine/coroutine.h>
///   using namespace sk::coro;
///
///   // Generator: produce a lazy sequence
///   Generator<int> range(int n) {
///     for (int i = 0; i < n; ++i) co_yield i;
///   }
///
///   // Task: async computation
///   Task<int> add_async(int a, int b) { co_return a + b; }
///
///   Task<int> compute() {
///     auto x = co_await add_async(1, 2);   // compose tasks
///     co_return x * 10;
///   }
///
///   int main() {
///     for (auto v : range(5)) std::cout << v << " ";
///     int result = sync_await(compute());
///   }

#include "event_loop.h"
#include "generator.h"
#include "sync_await.h"
#include "task.h"

// io_uring support is optional (Linux only, requires liburing)
#ifdef SK_HAS_IO_URING
#include "io_uring_async.h"
#endif

#endif  // SK_UTILS_COROUTINE_COROUTINE_H
