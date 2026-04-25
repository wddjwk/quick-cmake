/// demo_coro_io_uring.cpp — io_uring async IO with coroutines
///
/// This example shows how io_uring + coroutines enable truly
/// asynchronous file IO on Linux. io_uring is a Linux kernel
/// interface for async IO that avoids the overhead of traditional
/// blocking IO syscalls.
///
/// Key concepts:
///   - io_uring submits IO requests to a kernel ring buffer
///   - Completions arrive on a separate completion ring
///   - Our IoUringContext bridges io_uring completions to coroutine
///     resumption via the event loop
///   - Each async operation (read/write/fsync) is an Awaitable
///
/// Requires: Linux 5.1+, liburing-dev
/// Build: automatically enabled when liburing is detected

#include "config.h"

#ifdef SK_HAS_IO_URING

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "skutils/coroutine/coroutine.h"
#include "skutils/logger.h"

using namespace sk::coro;
using namespace sk::coro::io_uring;

// ============================================================
// Async file read using io_uring
// ============================================================

Task<std::string> async_read_file(IoUringContext& ctx, const std::string& path) {
  SK_LOG("  async_read_file: opening {}", path);

  // Open file (sync — in production you'd use io_uring for open too)
  int fd = ::open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    throw std::runtime_error("Failed to open " + path + ": " + std::strerror(errno));
  }

  // Get file size
  struct stat st;
  fstat(fd, &st);

  // Read the file via io_uring
  std::vector<char> buffer(st.st_size + 1, '\0');
  auto result = co_await ctx.async_read(fd, buffer.data(), st.st_size, 0);

  ::close(fd);

  if (!result.ok()) {
    throw std::runtime_error("io_uring read failed: error " + std::to_string(result.error_code));
  }

  SK_LOG("  async_read_file: read {} bytes from {}", result.bytes, path);
  co_return std::string(buffer.data(), result.bytes);
}

// ============================================================
// Async file write using io_uring
// ============================================================

Task<void> async_write_file(IoUringContext& ctx, const std::string& path, const std::string& content) {
  SK_LOG("  async_write_file: opening {}", path);

  int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    throw std::runtime_error("Failed to open " + path + ": " + std::strerror(errno));
  }

  auto result = co_await ctx.async_write(fd, content.data(), content.size(), 0);
  co_await ctx.async_fsync(fd);

  ::close(fd);

  if (!result.ok()) {
    throw std::runtime_error("io_uring write failed: error " + std::to_string(result.error_code));
  }

  SK_LOG("  async_write_file: wrote {} bytes to {}", result.bytes, path);
}

// ============================================================
// Copy file using io_uring coroutines
// ============================================================

Task<void> async_copy_file(IoUringContext& ctx, const std::string& src, const std::string& dst) {
  SK_LOG("=== Async file copy with io_uring ===");

  // Read the source file
  auto content = co_await async_read_file(ctx, src);
  SK_LOG("  Read {} bytes from {}", content.size(), src);

  // Write to destination
  co_await async_write_file(ctx, dst, content);
  SK_LOG("  Written to {}", dst);

  // Verify by reading back
  auto verify = co_await async_read_file(ctx, dst);
  SK_LOG("  Verification: {} bytes read back", verify.size());
  SK_LOG("  Content matches: {}", content == verify);
}

// ============================================================
// Multiple concurrent reads
// ============================================================

Task<std::vector<std::string>> read_multiple_files(IoUringContext& ctx, const std::vector<std::string>& paths) {
  // Note: true concurrency requires submitting all reads before awaiting.
  // This example reads sequentially for simplicity.
  // In a real system, you'd submit all SQEs, then await all CQEs.
  std::vector<std::string> results;
  for (const auto& path : paths) {
    try {
      auto content = co_await async_read_file(ctx, path);
      results.push_back(std::move(content));
    } catch (const std::exception& e) {
      SK_LOG("  Error reading {}: {}", path, e.what());
      results.push_back("[ERROR]");
    }
  }
  co_return results;
}

// ============================================================
// Main
// ============================================================

int main() {
  SK_LOG("=== io_uring Coroutine Demo ===");

  // Create a temp test file
  const std::string test_src = "/tmp/sk_coro_test_src.txt";
  const std::string test_dst = "/tmp/sk_coro_test_dst.txt";

  int fd = ::open(test_src.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  const std::string test_content = "Hello from sk::coro io_uring!\nThis is async IO.\n";
  ::write(fd, test_content.data(), test_content.size());
  ::close(fd);

  SK_LOG("Created test file: {}", test_src);

  // Set up io_uring context
  IoUringContext ctx(32);
  ctx.start();

  // Run the async copy
  auto task = async_copy_file(ctx, test_src, test_dst);
  sync_await(std::move(task));

  // Read multiple files
  SK_LOG("\n=== Reading multiple files ===");
  auto results = sync_await(read_multiple_files(ctx, {test_src, test_dst, "/etc/hostname"}));
  for (size_t i = 0; i < results.size(); ++i) {
    SK_LOG("  File {} result: {} bytes", i, results[i].size());
  }

  ctx.stop();

  // Cleanup
  ::unlink(test_src.c_str());
  ::unlink(test_dst.c_str());

  SK_LOG("Done!");
  return 0;
}

#else  // !SK_HAS_IO_URING

#include <iostream>

int main() {
  std::cout << "io_uring example not available.\n"
            << "This requires Linux with liburing installed.\n"
            << "Install: sudo apt install liburing-dev\n";
  return 0;
}

#endif  // SK_HAS_IO_URING
