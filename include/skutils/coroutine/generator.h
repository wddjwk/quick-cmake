#ifndef SK_UTILS_COROUTINE_GENERATOR_H
#define SK_UTILS_COROUTINE_GENERATOR_H

#include <coroutine>
#include <exception>
#include <iterator>
#include <type_traits>
#include <utility>

namespace sk::coro {

/// Generator<T> — lazy sequence producer via co_yield.
///
/// Usage:
///   Generator<int> fib(int n) {
///     int a = 0, b = 1;
///     for (int i = 0; i < n; ++i) {
///       co_yield a;
///       auto tmp = a; a = b; b = tmp + b;
///     }
///   }
///   for (auto v : fib(10)) { ... }
///
/// The coroutine body executes lazily: each dereference of the iterator
/// resumes the coroutine until the next co_yield or co_return.
template <typename T>
class Generator {
  public:
  struct promise_type;
  using HandleType = std::coroutine_handle<promise_type>;

  // --- Iterator for range-for support ---
  class Iterator {
public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using reference = const T&;
    using pointer = const T*;

    Iterator() noexcept = default;

    explicit Iterator(HandleType handle) noexcept : handle_(handle) {}

    reference operator*() const noexcept { return handle_.promise().current_value_; }

    pointer operator->() const noexcept { return &handle_.promise().current_value_; }

    Iterator& operator++() {
      handle_.resume();
      // Propagate any exception thrown inside the coroutine
      if (handle_.promise().exception_) {
        std::rethrow_exception(handle_.promise().exception_);
      }
      if (handle_.done()) {
        handle_ = nullptr;
      }
      return *this;
    }

    void operator++(int) { ++(*this); }

    friend bool operator==(const Iterator& lhs, const Iterator& rhs) noexcept { return lhs.handle_ == rhs.handle_; }

    friend bool operator!=(const Iterator& lhs, const Iterator& rhs) noexcept { return !(lhs == rhs); }

private:
    HandleType handle_ = nullptr;
  };

  // --- promise_type (required by the compiler) ---
  struct promise_type {
    T current_value_;
    std::exception_ptr exception_;

    Generator get_return_object() { return Generator{HandleType::from_promise(*this)}; }

    // Lazy: don't start executing until the first iterator dereference
    auto initial_suspend() { return std::suspend_always{}; }

    // Suspend at final suspend so the coroutine frame stays alive
    // until the Generator object is destroyed
    auto final_suspend() noexcept { return std::suspend_always{}; }

    void return_void() {}

    void unhandled_exception() { exception_ = std::current_exception(); }

    // co_yield value — store the yielded value and suspend
    auto yield_value(T value) {
      current_value_ = std::move(value);
      return std::suspend_always{};
    }

    // Support co_yield of convertible types
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
    auto yield_value(U&& value) {
      current_value_ = static_cast<T>(std::forward<U>(value));
      return std::suspend_always{};
    }
  };

  // --- Generator interface ---

  Generator() noexcept = default;

  explicit Generator(HandleType handle) noexcept : handle_(handle) {}

  Generator(const Generator&) = delete;
  Generator& operator=(const Generator&) = delete;

  Generator(Generator&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }

  Generator& operator=(Generator&& other) noexcept {
    if (this != &other) {
      if (handle_) {
        handle_.destroy();
      }
      handle_ = other.handle_;
      other.handle_ = nullptr;
    }
    return *this;
  }

  ~Generator() {
    if (handle_) {
      handle_.destroy();
    }
  }

  /// Begin iteration — resumes the coroutine to produce the first value
  Iterator begin() {
    if (!handle_) {
      return Iterator{};
    }
    handle_.resume();
    // Propagate any exception thrown during the first resume
    if (handle_.promise().exception_) {
      std::rethrow_exception(handle_.promise().exception_);
    }
    if (handle_.done()) {
      return Iterator{};
    }
    return Iterator{handle_};
  }

  /// Sentinel end iterator
  Iterator end() noexcept { return Iterator{}; }

  private:
  HandleType handle_ = nullptr;
};

}  // namespace sk::coro

#endif  // SK_UTILS_COROUTINE_GENERATOR_H
