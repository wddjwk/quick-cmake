#ifndef SHUAIKAI_UTILS_TIME_UTILS_H
#define SHUAIKAI_UTILS_TIME_UTILS_H

#include <chrono>
#include <ctime>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>

namespace sk::utils::time {

inline std::string current(const char *format = "%Y-%m-%d %H:%M:%S") {
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

  std::time_t time = std::chrono::system_clock::to_time_t(now);
  std::tm *local_time = std::localtime(&time);
  std::stringstream ss;
  ss << std::put_time(local_time, format);
  return ss.str();
}

template <typename Func, typename... Args>
auto cal_func_time(Func &&f, Args &&...args) {
  auto start = std::chrono::system_clock::now();
  auto result = std::invoke(std::forward<Func>(f), std::forward<Args>(args)...);
  // Prevent the compiler from optimizing away the result
#if defined(_MSC_VER)
  _ReadWriteBarrier();
#else
  asm volatile("" : : "r,m"(result) : "memory");
#endif
  auto end = std::chrono::system_clock::now();
  auto cnt = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  return cnt;
}
}  // namespace sk::utils::time

#endif  // SHUAIKAI_UTILS_TIME_UTILS_H
