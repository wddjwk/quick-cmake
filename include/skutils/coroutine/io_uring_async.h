#ifndef SK_UTILS_COROUTINE_IO_URING_ASYNC_H
#define SK_UTILS_COROUTINE_IO_URING_ASYNC_H

#include <coroutine>
#include <functional>

// This header requires Linux with io_uring support.
// It is only compiled when SK_HAS_IO_URING is defined.
#ifdef SK_HAS_IO_URING

#include <liburing.h>

#include "event_loop.h"

namespace sk::coro::io_uring {

// Forward declarations for awaiter types
class AsyncReadAwaiter;
class AsyncWriteAwaiter;
class AsyncFsyncAwaiter;

/// IoUringContext — owns a shared io_uring instance and its event loop.
///
/// This is the central coordination point between io_uring completions
/// and the coroutine event loop. When an io_uring operation completes,
/// the completion callback posts a resume to the event loop.
///
/// Usage:
///   IoUringContext ctx(32);   // queue depth 32
///   ctx.start();              // starts the polling thread
///   // ... submit async operations ...
///   ctx.stop();
class IoUringContext {
  public:
  explicit IoUringContext(unsigned entries = 64) { io_uring_queue_init(entries, &ring_, 0); }

  ~IoUringContext() { io_uring_queue_exit(&ring_); }

  IoUringContext(const IoUringContext&) = delete;
  IoUringContext& operator=(const IoUringContext&) = delete;

  /// Get the raw io_uring handle (for advanced use).
  struct io_uring* ring() { return &ring_; }

  /// Get the associated event loop.
  EventLoop& event_loop() { return loop_; }

  /// Submit an async read operation. Returns an Awaitable.
  /// The coroutine will be suspended until the read completes.
  AsyncReadAwaiter async_read(int fd, void* buffer, size_t size, off_t offset = 0);

  /// Submit an async write operation. Returns an Awaitable.
  AsyncWriteAwaiter async_write(int fd, const void* buffer, size_t size, off_t offset = 0);

  /// Submit an async fsync.
  AsyncFsyncAwaiter async_fsync(int fd);

  /// Poll for completed io_uring operations and post their
  /// completions to the event loop. Call this periodically.
  void poll_completions() {
    struct io_uring_cqe* cqe = nullptr;
    while (true) {
      auto ret = io_uring_peek_cqe(&ring_, &cqe);
      if (ret < 0) {
        break;  // no more completions
      }
      auto* callback = static_cast<std::function<void(int)>*>(io_uring_cqe_get_data(cqe));
      if (callback) {
        (*callback)(cqe->res);
        delete callback;
      }
      io_uring_cqe_seen(&ring_, cqe);
    }
  }

  /// Start the background completion-polling thread.
  void start() {
    running_ = true;
    thread_ = std::thread([this] {
      while (running_) {
        struct io_uring_cqe* cqe = nullptr;
        auto ret = io_uring_wait_cqe(&ring_, &cqe);
        if (ret < 0)
          continue;
        auto* callback = static_cast<std::function<void(int)>*>(io_uring_cqe_get_data(cqe));
        if (callback) {
          // Invoke the completion callback directly in the polling thread.
          // The callback will resume the suspended coroutine, which continues
          // execution on this background thread until it suspends again or
          // completes. This avoids the need to run a separate event loop.
          (*callback)(cqe->res);
          delete callback;
        }
        io_uring_cqe_seen(&ring_, cqe);
      }
    });
  }

  /// Stop the polling thread.
  void stop() {
    running_ = false;
    // Submit a NOP to wake up the waiting thread
    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    if (sqe) {
      io_uring_prep_nop(sqe);
      io_uring_submit(&ring_);
    }
    if (thread_.joinable()) {
      thread_.join();
    }
    loop_.stop();
  }

  private:
  struct io_uring ring_;
  EventLoop loop_;
  std::atomic<bool> running_{false};
  std::thread thread_;
};

/// Result of an async IO operation
struct IoResult {
  int bytes;       // bytes transferred (or negative error code)
  int error_code;  // 0 on success, errno-like on failure

  bool ok() const { return bytes >= 0; }
};

// ============================================================
// Awaitable: async read via io_uring
// ============================================================
class AsyncReadAwaiter {
  public:
  bool await_ready() noexcept { return false; }

  void await_suspend(std::coroutine_handle<> handle) {
    auto* sqe = io_uring_get_sqe(ctx_->ring());
    if (!sqe) {
      result_.bytes = -1;
      result_.error_code = EAGAIN;
      handle.resume();
      return;
    }
    io_uring_prep_read(sqe, fd_, buffer_, size_, offset_);

    auto* callback = new std::function<void(int)>([handle, this](int res) {
      result_.bytes = res;
      result_.error_code = (res < 0) ? -res : 0;
      handle.resume();
    });
    io_uring_sqe_set_data(sqe, callback);
    io_uring_submit(ctx_->ring());
  }

  IoResult await_resume() noexcept { return result_; }

  private:
  friend class IoUringContext;

  AsyncReadAwaiter(IoUringContext* ctx, int fd, void* buffer, size_t size, off_t offset)
    : ctx_(ctx), fd_(fd), buffer_(buffer), size_(size), offset_(offset) {}

  IoUringContext* ctx_;
  int fd_;
  void* buffer_;
  size_t size_;
  off_t offset_;
  IoResult result_{0, 0};
};

// ============================================================
// Awaitable: async write via io_uring
// ============================================================
class AsyncWriteAwaiter {
  public:
  bool await_ready() noexcept { return false; }

  void await_suspend(std::coroutine_handle<> handle) {
    auto* sqe = io_uring_get_sqe(ctx_->ring());
    if (!sqe) {
      result_.bytes = -1;
      result_.error_code = EAGAIN;
      handle.resume();
      return;
    }
    io_uring_prep_write(sqe, fd_, buffer_, size_, offset_);

    auto* callback = new std::function<void(int)>([handle, this](int res) {
      result_.bytes = res;
      result_.error_code = (res < 0) ? -res : 0;
      handle.resume();
    });
    io_uring_sqe_set_data(sqe, callback);
    io_uring_submit(ctx_->ring());
  }

  IoResult await_resume() noexcept { return result_; }

  private:
  friend class IoUringContext;

  AsyncWriteAwaiter(IoUringContext* ctx, int fd, const void* buffer, size_t size, off_t offset)
    : ctx_(ctx), fd_(fd), buffer_(buffer), size_(size), offset_(offset) {}

  IoUringContext* ctx_;
  int fd_;
  const void* buffer_;
  size_t size_;
  off_t offset_;
  IoResult result_{0, 0};
};

// ============================================================
// Awaitable: async fsync via io_uring
// ============================================================
class AsyncFsyncAwaiter {
  public:
  bool await_ready() noexcept { return false; }

  void await_suspend(std::coroutine_handle<> handle) {
    auto* sqe = io_uring_get_sqe(ctx_->ring());
    if (!sqe) {
      result_.bytes = -1;
      result_.error_code = EAGAIN;
      handle.resume();
      return;
    }
    io_uring_prep_fsync(sqe, fd_, 0);

    auto* callback = new std::function<void(int)>([handle, this](int res) {
      result_.bytes = res;
      result_.error_code = (res < 0) ? -res : 0;
      handle.resume();
    });
    io_uring_sqe_set_data(sqe, callback);
    io_uring_submit(ctx_->ring());
  }

  IoResult await_resume() noexcept { return result_; }

  private:
  friend class IoUringContext;

  AsyncFsyncAwaiter(IoUringContext* ctx, int fd) : ctx_(ctx), fd_(fd) {}

  IoUringContext* ctx_;
  int fd_;
  IoResult result_{0, 0};
};

// ============================================================
// IoUringContext method implementations (depend on Awaiter types)
// ============================================================

inline AsyncReadAwaiter IoUringContext::async_read(int fd, void* buffer, size_t size, off_t offset) {
  return AsyncReadAwaiter(this, fd, buffer, size, offset);
}

inline AsyncWriteAwaiter IoUringContext::async_write(int fd, const void* buffer, size_t size, off_t offset) {
  return AsyncWriteAwaiter(this, fd, buffer, size, offset);
}

inline AsyncFsyncAwaiter IoUringContext::async_fsync(int fd) {
  return AsyncFsyncAwaiter(this, fd);
}

}  // namespace sk::coro::io_uring

#endif  // SK_HAS_IO_URING

#endif  // SK_UTILS_COROUTINE_IO_URING_ASYNC_H
