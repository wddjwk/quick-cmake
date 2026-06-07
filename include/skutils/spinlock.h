#ifndef SHUAIKAI_SPINLOCK_H
#define SHUAIKAI_SPINLOCK_H

#include <atomic>

#include "noncopyable.h"

namespace sk::utils {

class SpinLock : public NonCopyable {
  private:
  std::atomic_flag flag;

  public:
  SpinLock() = default;

  void lock() {
    while (flag.test_and_set(std::memory_order_acquire)) {}
  }

  void unlock() { flag.clear(); }
};

class SpinLockGuard {
  private:
  SpinLock &lock;

  public:
  explicit SpinLockGuard(SpinLock &lck) : lock(lck) { lock.lock(); }

  ~SpinLockGuard() { lock.unlock(); }

  SpinLockGuard(const SpinLockGuard &) = delete;
  SpinLockGuard operator=(const SpinLockGuard &) = delete;
};

// inline SpinLock globalLogSpinLock;  // global cout lock

}  // namespace sk::utils

#endif  // SHUAIKAI_SPINLOCK_H
