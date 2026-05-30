/// @file skutils.h
/// @brief Single header-only library merging printer, logger, and all dependencies.
///        Just `#include "skutils.h"` — no other headers needed.
///
/// AUTO-GENERATED — do not edit manually. Modify source headers and rebuild.
/// Merged from: noncopyable.h, spinlock.h, config.h, string_utils.h, time_utils.h, printer.h, logger.h

#ifndef SK_HEADER_ONLY_SKUTILS_H
#define SK_HEADER_ONLY_SKUTILS_H

// ============================================================================
// Standard library includes
// ============================================================================

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

// ============================================================================
// NonCopyable
// ============================================================================

class NonCopyable {
  public:
  NonCopyable() = default;
  ~NonCopyable() = default;

  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;

  NonCopyable(NonCopyable&&) = default;
  NonCopyable& operator=(NonCopyable&&) = default;
};


// ============================================================================
// SpinLock
// ============================================================================

namespace sk::utils {

class SpinLock : public NonCopyable {
  private:
  std::atomic_flag flag;

  public:
  SpinLock() : flag{false} {}

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


// ============================================================================
// Config
// ============================================================================

namespace sk::utils {

#define ELEM_SEP ","
#define DUMP_SEP "\n"

#define SK_LOG_FOR_DEBUG 1  // set to 1 to print line infomation in log

#define UNKNOWN_TYPE_STRING "<?>"

enum class OSTYPE { Windows, Linux, Unknown };

inline constexpr OSTYPE OS_TYPE() {
#if defined(_WIN32) || defined(_WIN64)
  return OSTYPE::Windows;
#elif defined(__linux__)
  return OSTYPE::Linux;
#else
  return OSTYPE::Unknown;
#endif
}

inline constexpr bool IS_LINUX_OS() {
  return OS_TYPE() == OSTYPE::Linux;
}

class GlobalInfo : public NonCopyable {
  private:
  GlobalInfo() : gFailedTest{0}, gTotalTest{0}, gDemoId{0} {};

  public:
  SpinLock globalLogSpinLock;
  std::atomic<int> gFailedTest;
  std::atomic<int> gTotalTest;
  std::atomic<int> gDemoId;

  static GlobalInfo& getInstance() {
    static GlobalInfo instance;
    return instance;
  }
};

}  // namespace sk::utils


// ============================================================================
// String Utilities
// ============================================================================

namespace sk::utils::str {
inline bool startWith(std::string_view str, std::string_view prefix) {
#if __cplusplus >= 202002L
  return str.starts_with(prefix);
#else
  if (str.length() < prefix.length()) {
    return false;
  }
  for (int i = 0; i < prefix.length(); i++) {
    if (prefix[i] != str[i]) {
      return false;
    }
  }
  return true;
#endif
}

inline bool endWith(std::string_view str, std::string_view suffix) {
#if __cplusplus >= 202002L
  return str.ends_with(suffix);
#else
  if (str.length() < suffix.length()) {
    return false;
  }
  auto offset = str.length() - suffix.length();
  for (int i = 0; i < suffix.length(); i++) {
    if (str[i + offset] != suffix[i]) {
      return false;
    }
  }
  return true;
#endif
}

inline std::string strip(std::string_view str) {
  auto l = str.find_first_not_of(' ');
  auto r = str.find_last_not_of(' ');
  return std::string(str.substr(l, r - l + 1));
}

inline bool isspace(char c) {
  return c == ' ' || c == '\n' || c == '\t';
}

inline bool isspace(std::string_view s) {
  return std::all_of(s.begin(), s.end(), [](char c) { return isspace(c); });
}

inline bool contains(std::string_view str, std::string_view substr) {
  return str.find(substr) != std::string::npos;
}

inline int constexpr count(std::string_view str, std::string_view substr) {
  int cnt = 0;
  size_t pos = 0;
  auto sublength = substr.length();
  while ((pos = str.find(substr, pos)) != std::string::npos) {
    cnt++;
    pos += sublength;
  }
  return cnt;
}

template <typename IterMoter>
inline auto _split(std::string_view str, IterMoter itermoter) -> typename std::enable_if<
  std::is_same<decltype(itermoter(std::declval<std::string_view>())), std::pair<bool, decltype(str.length())>>::value,
  std::vector<std::string>>::type {
  auto len = str.length();
  decltype(len) idx = 0;
  decltype(len) starter = 0;
  std::vector<std::string> ret;
  while (idx < len) {
    auto [isDelim, idxInstance] = itermoter(str.substr(idx, len - idx + 1));
    if (isDelim) {
      if (idx > starter) {
        ret.emplace_back(str.substr(starter, idx - starter));
      }
      idx += idxInstance;
      starter = idx;
    } else {
      ++idx;
    }
  }
  if (starter < len) {
    ret.emplace_back(str.substr(starter, len - starter + 1));
  }
  return ret;
}

inline std::vector<std::string> split(std::string_view str, char delim) {
  return _split(str, [&delim](std::string_view substr) -> std::pair<bool, decltype(str.length())> {
    if (substr[0] == delim) {
      return {true, 1};
    }
    return {false, 0};
  });
}

inline std::vector<std::string> split(std::string_view str, std::string_view delim) {
  return _split(str, [&](std::string_view substr) -> std::pair<bool, decltype(str.length())> {
    if (startWith(substr, delim)) {
      return {true, delim.length()};
    }
    return {false, 0};
  });
}

inline std::vector<std::string> split(std::string_view str, std::vector<std::string> delims) {
  std::sort(delims.begin(), delims.end(), std::greater<>());
  return _split(str, [&](std::string_view substr) -> std::pair<bool, decltype(str.length())> {
    for (const auto& delim : delims) {
      if (startWith(substr, delim)) {
        return {true, delim.length()};
      }
    }
    return {false, 0};
  });
}

inline std::vector<std::string> split(std::string_view str, std::vector<char> delims) {
  std::sort(delims.begin(), delims.end(), std::greater<>());
  return _split(str, [&](std::string_view substr) -> std::pair<bool, decltype(str.length())> {
    for (char c : delims) {
      if (substr[0] == c) {
        return {true, 1};
      }
    }
    return {false, 0};
  });
}

inline std::string replace(const std::string& str, const std::string& pattern, const std::string& replacer) {
  std::regex re(pattern);
  return std::regex_replace(str, re, replacer);
}

inline std::string replace(std::string&& str, std::string&& pattern, std::string&& replacer) {
  std::regex re(pattern);
  return std::regex_replace(str, re, replacer);
}

inline std::string trim(const std::string str, const std::string trimer = " ") {
  return replace(str, trimer, "");
}

inline std::string dirname(std::string_view filename) {
  auto pos = filename.find_last_of('/');
  if (pos == std::string::npos) {
    pos = filename.find_last_of('\\');
  }
  return std::string(filename.substr(0, pos));
}

inline std::string basename(std::string_view filename) {
  return *(split(filename, std::vector<std::string>{"/", "\\"}).end() - 1);
}

inline std::string basenameWithoutExt(std::string_view filename) {
  auto base = basename(filename);
  return base.substr(0, base.find_last_of('.'));
}

inline std::string expandUser(std::string_view path) {
  if (!startWith(path, "~")) {
    return std::string(path);
  }
  char* home_dir = nullptr;
#if defined(_WIN32)
  home_dir = getenv("USERPROFILE");
#else
  home_dir = getenv("HOME");
#endif

  return replace(std::string(path), "~", std::string(home_dir));
}

}  // namespace sk::utils::str


// ============================================================================
// Time Utilities
// ============================================================================

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
  // refer to benchmark::DoNotOptimize
  asm volatile("" : : "r,m"(std::invoke(std::forward<Func>(f), std::forward<Args>(args)...)) : "memory");
  auto end = std::chrono::system_clock::now();
  auto cnt = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  return cnt;
}
}  // namespace sk::utils::time


// ============================================================================
// Printer
// ============================================================================

#define GUARD_LOG sk::utils::SpinLockGuard guard(sk::utils::GlobalInfo::getInstance().globalLogSpinLock)

#define REPLACED_SEP(s) sk::utils::str::replace((s), ",", ELEM_SEP)

/// MARK: COLOR

#define ANSI_CLEAR "\033[0m"
#define ANSI_RED_BG "\033[0;31m"
#define ANSI_GREEN_BG "\033[0;32m"
#define ANSI_YELLOW_BG "\033[0;33m"
#define ANSI_BLUE_BG "\033[0;34m"
#define ANSI_PURPLE_BG "\033[0;35m"
#define ANSI_GRAY_BG "\033[38;5;246m"
#define ANSI_BOLD "\033[1m"
#define ANSI_ITALIC "\033[3m"
#define ANSI_UNDERLINE "\033[4m"

#define ANSI_TEMPLATE_COLOR "\033[0m"
#define ANSI_KEY_COLOR "\033[0m\033[3m"

#define WITH_RED(x) "\033[0m\033[0;31m" + sk::utils::toString(x) + "\033[0m"
#define WITH_GREEN(x) "\033[0m\033[0;32m" + sk::utils::toString(x) + "\033[0m"
#define WITH_YELLOW(x) "\033[0m\033[0;33m" + sk::utils::toString(x) + "\033[0m"
#define WITH_BLUE(x) "\033[0m\033[0;34m" + sk::utils::toString(x) + "\033[0m"
#define WITH_PURPLE(x) "\033[0m\033[0;35m" + sk::utils::toString(x) + "\033[0m"
#define WITH_GRAY(x) "\033[0m\033[38;5;246m" + sk::utils::toString(x) + "\033[0m"
#define WITH_BOLD(x) "\033[0m\033[1m" + sk::utils::toString(x) + "\033[0m"
#define WITH_ITALIC(x) "\033[0m\033[3m" + sk::utils::toString(x) + "\033[0m"

/// MARK: Concepts

namespace sk::utils {

#if __cplusplus >= 202002L

template <typename T>
concept Serializable = requires(T obj) {
  { obj.toString() } -> std::convertible_to<std::string_view>;
};

template <typename T>
concept StreamOutable = requires(std::ostream &os, T elem) {
  { os << elem } -> std::same_as<std::ostream &>;
};

template <typename T>
concept SequentialContainer = requires(T c) {
  typename T::value_type;
  { c.cbegin() } -> std::same_as<typename T::const_iterator>;
  { c.cend() } -> std::same_as<typename T::const_iterator>;
};

template <typename T>
concept MappedContainer = requires(T m) {
  typename T::key_type;
  typename T::mapped_type;
  { m.cbegin() } -> std::same_as<typename T::const_iterator>;
  { m.cend() } -> std::same_as<typename T::const_iterator>;
};

template <typename T>
concept StackLike = requires(T m) {
  typename T::value_type;
  { m.pop() } -> std::same_as<void>;
  { m.top() } -> std::convertible_to<typename T::const_reference>;
  { m.empty() } -> std::same_as<bool>;
};

template <typename T>
concept QueueLike = requires(T m) {
  typename T::value_type;
  { m.pop() } -> std::same_as<void>;
  { m.front() } -> std::convertible_to<typename T::const_reference>;
  { m.empty() } -> std::same_as<bool>;
};

template <typename T>
concept PairLike = requires(T p) {
  { std::get<0>(p) } -> std::convertible_to<typename T::first_type>;
  { std::get<1>(p) } -> std::convertible_to<typename T::second_type>;
};

template <typename T>
concept SmartPointer = requires(T p) {
  typename T::element_type;
  { p.get() };
  { static_cast<bool>(p) };
};

template <typename T>
concept OptionalLike = requires(T o) {
  { o.has_value() } -> std::same_as<bool>;
  { *o };
} && !SmartPointer<T>;

template <typename T>
struct is_variant_impl : std::false_type {};
template <typename... Ts>
struct is_variant_impl<std::variant<Ts...>> : std::true_type {};
template <typename T>
concept VariantType = is_variant_impl<std::remove_cv_t<T>>::value;

template <typename T>
concept TupleLike = requires {
  { std::tuple_size<T>::value } -> std::convertible_to<std::size_t>;
} && !PairLike<T> && !SequentialContainer<T>;

template <typename T>
concept Printable = StreamOutable<T> || Serializable<T> || SequentialContainer<T> || MappedContainer<T> || PairLike<T>
                    || StackLike<T> || QueueLike<T> || SmartPointer<T> || OptionalLike<T> || VariantType<T>
                    || TupleLike<T> || std::is_enum_v<T>;

/// MARK: Printer

template <Printable T>
auto toString(const T &obj) -> std::string;

template <typename T>
  requires Printable<typename T::value_type>
auto forBasedContainer2String(const T &c);

template <SequentialContainer T>
  requires Printable<typename T::value_type>
auto SequentialContainer2String(const T &c);

template <PairLike T>
  requires Printable<typename T::first_type> && Printable<typename T::second_type>
auto Pair2String(const T &p);

template <MappedContainer T>
  requires Printable<typename T::key_type> && Printable<typename T::mapped_type>
auto MappedContainer2String(const T &c);

template <StackLike T>
  requires Printable<typename T::value_type>
auto Stack2String(const T &c);

template <QueueLike T>
  requires Printable<typename T::value_type>
auto Queue2String(const T &c);

template <TupleLike T>
auto Tuple2String(const T &t);

/// MARK: Printer Impl

template <PairLike T>
  requires Printable<typename T::first_type> && Printable<typename T::second_type>

auto Pair2String(const T &p) {
  std::stringstream ss;
  ss << '{' << toString(std::get<0>(p)) << ELEM_SEP << toString(std::get<1>(p)) << "}";
  return ss.str();
}

template <typename T>
  requires Printable<typename T::value_type>

auto forBasedContainer2String(const T &c) {
  if (c.empty()) {
    return std::string("[]");
  }
  // 会有类型问题？
  // std::accumulate(std::next(c.begin()), c.end(),
  // toString(*(c.begin())),
  // [](string a, auto b){return a + ELEM_SEP + toString(b);});

  std::stringstream ss;
  ss << "[";
  for (const auto &elem : c) {
    ss << toString(elem) << ELEM_SEP;
  }
  std::string ret = ss.str();
  constexpr size_t sep_len = sizeof(ELEM_SEP) - 1;
  for (size_t i = 0; i < sep_len; ++i) {
    ret.pop_back();
  }
  ret.push_back(']');
  return ret;
}

template <SequentialContainer T>
  requires Printable<typename T::value_type>

auto SequentialContainer2String(const T &c) {
  return forBasedContainer2String(c);
}

template <MappedContainer T>
  requires Printable<typename T::key_type> && Printable<typename T::mapped_type>

auto MappedContainer2String(const T &c) {
  return forBasedContainer2String(c);
}

template <StackLike T>
  requires Printable<typename T::value_type>

auto Stack2String(const T &c) {
  if (c.empty()) {
    return std::string("[]");
  }
  const std::string stack_sep = "<-";
  T tmp = c;
  std::stringstream ss;
  ss << "Stack[" << toString(tmp.top()) << stack_sep;
  tmp.pop();
  while (!tmp.empty()) {
    ss << toString(tmp.top()) << stack_sep;
    tmp.pop();
  }
  std::string ret = ss.str();
  for (int i = 0; i < stack_sep.length(); ++i) {
    ret.pop_back();
  }
  ret.append("]");
  return ret;
}

template <QueueLike T>
  requires Printable<typename T::value_type>

auto Queue2String(const T &c) {
  if (c.empty()) {
    return std::string("[]");
  }
  const std::string queue_sep = "<-";
  T tmp = c;
  std::stringstream ss;
  ss << "Queue[" << toString(tmp.front()) << queue_sep;
  tmp.pop();
  while (!tmp.empty()) {
    ss << toString(tmp.front()) << queue_sep;
    tmp.pop();
  }
  std::string ret = ss.str();
  for (int i = 0; i < queue_sep.length(); ++i) {
    ret.pop_back();
  }
  ret.append("]");
  return ret;
}

template <typename T, std::size_t... Is>
std::string tupleToStringImpl(const T &t, std::index_sequence<Is...>) {
  std::stringstream ss;
  ss << "(";
  ((ss << (Is == 0 ? "" : ELEM_SEP) << toString(std::get<Is>(t))), ...);
  ss << ")";
  return ss.str();
}

template <TupleLike T>
auto Tuple2String(const T &t) {
  return tupleToStringImpl(t, std::make_index_sequence<std::tuple_size_v<T>>{});
}

template <Printable T>
auto toString(const T &obj) -> std::string {
  if constexpr (Serializable<T>) {
    return obj.toString();
  } else if constexpr (std::is_same_v<T, bool>) {
    return obj ? "True" : "False";
  } else if constexpr (std::is_function_v<T>) {
    std::stringstream ss;
    ss << "<func@" << reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(&obj)) << ">";
    return ss.str();
  } else if constexpr (std::is_pointer_v<T> && !std::is_convertible_v<const char *, T>) {
    std::stringstream ss;
    if constexpr (std::is_function_v<std::remove_pointer_t<T>>) {
      ss << "<func@" << reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(obj)) << ">";
    } else {
      if (obj == nullptr) {
        ss << "nullptr";
      } else {
        ss << static_cast<const void *>(obj) << " => ";
        if constexpr (Printable<std::remove_reference_t<decltype(*obj)>>) {
          ss << toString(*obj);
        } else {
          ss << UNKNOWN_TYPE_STRING;
        }
      }
    }
    return ss.str();
  } else if constexpr (SmartPointer<T>) {
    std::stringstream ss;
    if (!obj) {
      ss << "nullptr";
    } else {
      ss << static_cast<const void *>(obj.get()) << " => " << toString(*obj);
    }
    return ss.str();
  } else if constexpr (OptionalLike<T>) {
    if (obj.has_value()) {
      using ValueType = std::decay_t<decltype(*obj)>;
      if constexpr (Printable<ValueType>) {
        return "optional(" + toString(*obj) + ")";
      } else {
        return "optional(" UNKNOWN_TYPE_STRING ")";
      }
    }
    return "nullopt";
  } else if constexpr (VariantType<T>) {
    return std::visit(
      [](const auto &val) -> std::string {
        using V = std::decay_t<decltype(val)>;
        if constexpr (Printable<V>) {
          return "variant(" + toString(val) + ")";
        } else {
          return "variant(" UNKNOWN_TYPE_STRING ")";
        }
      },
      obj);
  } else if constexpr (StreamOutable<T>) {
    std::stringstream ss;
    ss << obj;
    return ss.str();
  } else if constexpr (SequentialContainer<T>) {
    return SequentialContainer2String(obj);
  } else if constexpr (MappedContainer<T>) {
    return MappedContainer2String(obj);
  } else if constexpr (PairLike<T>) {
    return Pair2String(obj);
  } else if constexpr (TupleLike<T>) {
    return Tuple2String(obj);
  } else if constexpr (StackLike<T>) {
    return Stack2String(obj);
  } else if constexpr (QueueLike<T>) {
    return Queue2String(obj);
  } else if constexpr (std::is_enum_v<T>) {
    return "enum(" + std::to_string(static_cast<std::underlying_type_t<T>>(obj)) + ")";
  } else {
    GUARD_LOG;
    std::cerr << ANSI_RED_BG << "Isn't Printable\n" << ANSI_CLEAR;
    return UNKNOWN_TYPE_STRING;
  }
}

template <Printable... Args>
void dump(Args... args) {
  ((std::cout << toString(args) << " "), ...);
  std::cout << "\n";
}

template <PairLike... PairType>
void dumpWithName(PairType... args) {
  GUARD_LOG;
  ((std::cout << ANSI_PURPLE_BG << "[" << toString(std::get<0>(args)) << "]:" << ANSI_CLEAR
              << toString(std::get<1>(args)) << DUMP_SEP),
   ...);
}

#else  // __cplusplus >= 202002L

/// MARK: CXX 17 Version

using namespace std::string_literals;

template <typename T, typename = void>
struct Serializable : std::false_type {};

//* 判断类型属性，也可以使用 enable_if 来实现。如果不关心返回值，则可以使用 void_t
// template <typename T>
// struct Serializable<T, std::void_t<decltype(std::declval<T>().toString())>> : std::true_type {};
template <typename T>
struct Serializable<T, std::enable_if_t<std::is_convertible_v<decltype(std::declval<T>().toString()), std::string>>>
  : std::true_type {};

//* enable_if 作为模板类型来控制模板特化。（另一种常用的方式是作为函数返回值来控制函数特化）
// @Deprecated leetcodepointertype is deleted
// template <typename T>
// struct LeetCodePointerType<
//     T, std::enable_if_t<(std::is_same_v<ListNode *, T> || std::is_same_v<TreeNode *, T>)&&Serializable<T>::value,
//     void>> : std::true_type {};

template <typename T, typename = void>
struct StreamOutable : std::false_type {};

//* void_t 描述类型特质，具有xx属性
template <typename T>
struct StreamOutable<T, std::void_t<decltype(std::declval<std::ostream &>() << std::declval<T>())>> : std::true_type {};

template <typename T, typename = void>
struct SequentialContainer : std::false_type {};

//* 将 enable_if_t 嵌入到 void_t 中使用。std::void_t 可以接受 任意多 的类型，判断他们是否都存在
template <typename T>
struct SequentialContainer<
  T, std::void_t<typename T::value_type,
                 std::enable_if_t<std::is_same_v<decltype(std::declval<T>().empty()), bool>
                                  && std::is_same_v<decltype(std::declval<T>().cbegin()), typename T::const_iterator>
                                  && std::is_same_v<decltype(std::declval<T>().cend()), typename T::const_iterator>>>>
  : std::true_type {};

template <typename T, typename = void>
struct MappedContainer : std::false_type {};

template <typename T>
struct MappedContainer<
  T, std::void_t<typename T::key_type, typename T::mapped_type,
                 std::enable_if_t<std::is_same_v<decltype(std::declval<T>().cbegin()), typename T::const_iterator>
                                  && std::is_same_v<decltype(std::declval<T>().cend()), typename T::const_iterator>>>>
  : std::true_type {};

template <typename T, typename = void>
struct StackLike : std::false_type {};

//* 使用 std::conjunction 来 AND 判断多个类型的 value 是否为 true;
/**
 *   注意，std::void_t 和 std::conjunction 中都使用了 pop() 等成员函数，这并不赘余！
 *   不能觉得 std::conjunction 中有了就可以了。因为 void_t 中函数不存在时 `SFINAF`, 但是 conjunction
 *   中不存在相应的成员而应要declval则是属于 `编译错误`！
 *   因为conjuction属于继承父类，不是模板参数检查，所以也就不能 SFINAF
 */
template <typename T>
struct StackLike<T, std::void_t<typename T::value_type, decltype(std::declval<T>().pop()),
                                decltype(std::declval<T>().push(std::declval<typename T::value_type>())),
                                decltype(std::declval<T>().top()), decltype(std::declval<T>().empty())>>
  : std::conjunction<std::is_same<decltype(std::declval<T>().pop()), void>,
                     std::is_same<decltype(std::declval<T>().push(std::declval<typename T::value_type>())), void>,
                     std::is_same<decltype(std::declval<T>().top()), typename T::value_type>,
                     std::is_same<decltype(std::declval<T>().empty()), bool>> {};

// 当然，仍然可以使用嵌套 enable_if 的方式来做，看起来似乎更简洁
// template <typename T>
// struct StackLike<
//     T, std::void_t<
//            typename T::value_type,
//            std::enable_if_t<
//                std::is_same_v<decltype(std::declval<T>().pop()), void>
//                && std::is_same_v<decltype(std::declval<T>().empty(), bool)>
//                && std::is_same_v<decltype(std::declval<T>().push(std::declval<typename T::value_type>())), void>
//                && std::is_same_v<std::remove_reference_t<decltype(std::declval<T>().top())>, typename
//                T::value_type>>>>
//     : std::true_type {};

template <typename T, typename = void>
struct QueueLike : std::false_type {};

template <typename T>
struct QueueLike<
  T, std::void_t<typename T::value_type,
                 std::enable_if_t<std::is_same_v<decltype(std::declval<T>().empty()), bool>
                                  && std::is_convertible_v<decltype(std::declval<T>().front()), typename T::value_type>
                                  && std::is_same_v<decltype(std::declval<T>().pop()), void>>>> : std::true_type {};

template <typename T, typename = void>
struct PairLike : std::false_type {};

template <typename T>
struct PairLike<
  T, std::enable_if_t<std::is_convertible_v<decltype(std::get<0>(std::declval<T>())), typename T::first_type>
                      && std::is_convertible_v<decltype(std::get<1>(std::declval<T>())), typename T::second_type>>>
  : std::true_type {};

template <typename T, typename = void>
struct SmartPointer : std::false_type {};
template <typename T>
struct SmartPointer<T, std::void_t<typename T::element_type, decltype(std::declval<T>().get()),
                                   decltype(static_cast<bool>(std::declval<T>()))>> : std::true_type {};

template <typename T, typename = void>
struct OptionalLike : std::false_type {};
template <typename T>
struct OptionalLike<T, std::void_t<decltype(std::declval<T>().has_value()), decltype(*std::declval<T>()),
                                   std::enable_if_t<!SmartPointer<T>::value>>> : std::true_type {};

template <typename T>
struct IsVariant : std::false_type {};
template <typename... Ts>
struct IsVariant<std::variant<Ts...>> : std::true_type {};

template <typename T, typename = void>
struct TupleLike : std::false_type {};
template <typename T>
struct TupleLike<T, std::void_t<decltype(std::tuple_size<T>::value),
                                std::enable_if_t<!PairLike<T>::value && !SequentialContainer<T>::value>>>
  : std::true_type {};

template <typename T>
struct Printable : std::disjunction<StreamOutable<T>, Serializable<T>, SequentialContainer<T>, MappedContainer<T>,
                                    StackLike<T>, QueueLike<T>, PairLike<T>, SmartPointer<T>, OptionalLike<T>,
                                    IsVariant<std::remove_cv_t<T>>, TupleLike<T>, std::is_enum<T>> {};

//* enable_if 作为函数的参数，来限制函数模板选择
template <typename T>
auto toString(const T &obj) -> std::enable_if_t<Printable<T>::value, std::string>;

/// MARK: CXX17 Impl

//* enable_if 直接用作模板参数，注意有个等号
template <typename T, typename = std::enable_if_t<PairLike<T>::value && Printable<typename T::first_type>::value
                                                    && Printable<typename T::second_type>::value,
                                                  void>>
auto Pair2String(const T &p) {
  std::stringstream ss;
  ss << '{' << toString(std::get<0>(p)) << ELEM_SEP << toString(std::get<1>(p)) << "}";
  return ss.str();
}

template <typename T>
std::string forBasedContainer2String(const T &c) {
  if (c.empty()) {
    return "[]"s;
  }

  std::stringstream ss;
  ss << "[";
  for (const auto &elem : c) {
    ss << toString(elem) << ELEM_SEP;
  }
  std::string ret = ss.str();
  constexpr size_t sep_len = sizeof(ELEM_SEP) - 1;
  for (size_t i = 0; i < sep_len; ++i) {
    ret.pop_back();
  }
  ret.push_back(']');
  return ret;
}

//* enable_if 作为函数的参数，来限制函数模板选择
template <typename T>
auto SequentialContainer2String(const T &c)
  -> std::enable_if_t<SequentialContainer<T>::value && Printable<typename T::value_type>::value, std::string> {
  return forBasedContainer2String(c);
}

template <typename T>
auto MappedContainer2String(const T &c)
  -> std::enable_if_t<MappedContainer<T>::value && Printable<typename T::key_type>::value
                        && Printable<typename T::mapped_type>::value,
                      std::string> {
  return forBasedContainer2String(c);
}

template <typename T>
auto Stack2String(const T &c)
  -> std::enable_if_t<StackLike<T>::value && Printable<typename T::value_type>::value, std::string> {
  if (c.empty()) {
    return "[]"s;
  }
  const std::string stack_sep = "<-";
  T tmp = c;
  std::stringstream ss;
  ss << "Stack[" << toString(tmp.top()) << stack_sep;
  tmp.pop();
  while (!tmp.empty()) {
    ss << toString(tmp.top()) << stack_sep;
    tmp.pop();
  }
  std::string ret = ss.str();
  for (int i = 0; i < stack_sep.length(); ++i) {
    ret.pop_back();
  }
  ret.append("]");
  return ret;
}

template <typename T>
auto Queue2String(const T &c)
  -> std::enable_if_t<QueueLike<T>::value && Printable<typename T::value_type>::value, std::string> {
  if (c.empty()) {
    return "[]"s;
  }
  const std::string queue_sep = "<-";
  T tmp = c;
  std::stringstream ss;
  ss << "Queue[" << toString(tmp.front()) << queue_sep;
  tmp.pop();
  while (!tmp.empty()) {
    ss << toString(tmp.front()) << queue_sep;
    tmp.pop();
  }
  std::string ret = ss.str();
  for (int i = 0; i < queue_sep.length(); ++i) {
    ret.pop_back();
  }
  ret.append("]");
  return ret;
}

template <typename T, std::size_t... Is>
std::string tupleToStringImpl(const T &t, std::index_sequence<Is...>) {
  std::stringstream ss;
  ss << "(";
  ((ss << (Is == 0 ? "" : ELEM_SEP) << toString(std::get<Is>(t))), ...);
  ss << ")";
  return ss.str();
}

template <typename T>
auto Tuple2String(const T &t) -> std::enable_if_t<TupleLike<T>::value, std::string> {
  return tupleToStringImpl(t, std::make_index_sequence<std::tuple_size_v<T>>{});
}

template <typename T>
auto toString(const T &obj) -> std::enable_if_t<Printable<T>::value, std::string> {
  if constexpr (Serializable<T>::value) {
    return obj.toString();
  } else if constexpr (std::is_same_v<T, bool>) {
    return obj ? "True"s : "False"s;
  } else if constexpr (std::is_function_v<T>) {
    std::stringstream ss;
    ss << "<func@" << reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(&obj)) << ">";
    return ss.str();
  } else if constexpr (std::is_pointer_v<T> && !std::is_convertible_v<const char *, T>) {
    std::stringstream ss;
    if constexpr (std::is_function_v<std::remove_pointer_t<T>>) {
      ss << "<func@" << reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(obj)) << ">";
    } else {
      if (obj == nullptr) {
        ss << "nullptr";
      } else {
        ss << static_cast<const void *>(obj) << " => ";
        if constexpr (Printable<std::remove_reference_t<decltype(*obj)>>::value) {
          ss << toString(*obj);
        } else {
          ss << UNKNOWN_TYPE_STRING;
        }
      }
    }
    return ss.str();
  } else if constexpr (SmartPointer<T>::value) {
    std::stringstream ss;
    if (!obj) {
      ss << "nullptr";
    } else {
      ss << static_cast<const void *>(obj.get()) << " => " << toString(*obj);
    }
    return ss.str();
  } else if constexpr (OptionalLike<T>::value) {
    if (obj.has_value()) {
      using ValueType = std::decay_t<decltype(*obj)>;
      if constexpr (Printable<ValueType>::value) {
        return "optional(" + toString(*obj) + ")";
      } else {
        return "optional(" UNKNOWN_TYPE_STRING ")";
      }
    }
    return "nullopt";
  } else if constexpr (IsVariant<std::remove_cv_t<T>>::value) {
    return std::visit(
      [](const auto &val) -> std::string {
        using V = std::decay_t<decltype(val)>;
        if constexpr (Printable<V>::value) {
          return "variant(" + toString(val) + ")";
        } else {
          return "variant(" UNKNOWN_TYPE_STRING ")";
        }
      },
      obj);
  } else if constexpr (StreamOutable<T>::value) {
    std::stringstream ss;
    ss << obj;
    return ss.str();
  } else if constexpr (SequentialContainer<T>::value) {
    return SequentialContainer2String(obj);
  } else if constexpr (MappedContainer<T>::value) {
    return MappedContainer2String(obj);
  } else if constexpr (PairLike<T>::value) {
    return Pair2String(obj);
  } else if constexpr (TupleLike<T>::value) {
    return Tuple2String(obj);
  } else if constexpr (StackLike<T>::value) {
    return Stack2String(obj);
  } else if constexpr (QueueLike<T>::value) {
    return Queue2String(obj);
  } else if constexpr (std::is_enum_v<T>) {
    return "enum(" + std::to_string(static_cast<std::underlying_type_t<T>>(obj)) + ")";
  } else {
    GUARD_LOG;
    std::cerr << ANSI_RED_BG << "Isn't Printable\n" << ANSI_CLEAR;
    return UNKNOWN_TYPE_STRING;
  }
}

template <typename... Args, typename = std::enable_if_t<(Printable<Args>::value && ...), void>>
void dump(Args... args) {
  ((std::cout << toString(args) << " "), ...);
  std::cout << "\n";
}

//* 用 conjunction 来判断可变参数列表
template <typename... PairType, typename = std::enable_if_t<std::conjunction_v<PairLike<PairType>...>, void>>
void dumpWithName(PairType... args) {
  GUARD_LOG;
  ((std::cout << ANSI_PURPLE_BG << "[" << toString(std::get<0>(args)) << "]:" << ANSI_CLEAR
              << toString(std::get<1>(args)) << DUMP_SEP),
   ...);
}

#endif  // __cplusplus >= 202002L

// MARK: Formatter

template <typename... Args>
std::string format(std::string_view fmt, Args... args) {
  std::string fmtStr(fmt);
  if constexpr (!sizeof...(args)) {
    return fmtStr;
  } else {
    auto replace_next = [&fmtStr](const std::string &val) {
      auto pos = fmtStr.find("{}");
      if (pos != std::string::npos) {
        fmtStr.replace(pos, 2, val);
      }
    };
    (replace_next(toString(args)), ...);
    return fmtStr;
  }
}

template <typename... Args>
std::string colorful_format(std::string_view fmt, Args... args) {
  std::string fmtStr(fmt);
  if constexpr (!sizeof...(args)) {
    return ANSI_TEMPLATE_COLOR + fmtStr + ANSI_CLEAR;
  } else {
    auto replace_next = [&fmtStr](const std::string &val) {
      auto pos = fmtStr.find("{}");
      if (pos != std::string::npos) {
        fmtStr.replace(pos, 2, ANSI_KEY_COLOR + val + ANSI_TEMPLATE_COLOR);
      }
    };
    (replace_next(toString(args)), ...);
    return ANSI_TEMPLATE_COLOR + fmtStr + ANSI_CLEAR;
  }
}

template <typename... Args>
void print(std::string_view fmt, Args... args) {
  std::string fmtStr(fmt);
  if constexpr (!sizeof...(args)) {
    GUARD_LOG;
    std::cout << fmtStr;
  } else {
    auto ret = colorful_format(fmt, std::forward<Args>(args)...);
    GUARD_LOG;
    std::cout << ret;
  }
}

template <typename... Args>
void println(std::string_view fmt, Args... args) {
  std::string fmtStr(fmt);
  if constexpr (!sizeof...(args)) {
    GUARD_LOG;
    std::cout << fmtStr << "\n";
  } else {
    auto ret = colorful_format(fmt, std::forward<Args>(args)...);
    GUARD_LOG;
    std::cout << ret << "\n";
  }
}

}  // namespace sk::utils

#define TO_PAIR(x) std::make_pair(#x, x)

#define DUMP1(x) sk::utils::dumpWithName(TO_PAIR(x))
#define DUMP2(x, ...) sk::utils::dumpWithName(TO_PAIR(x)), DUMP1(__VA_ARGS__)
#define DUMP3(x, ...) sk::utils::dumpWithName(TO_PAIR(x)), DUMP2(__VA_ARGS__)
#define DUMP4(x, ...) sk::utils::dumpWithName(TO_PAIR(x)), DUMP3(__VA_ARGS__)
#define DUMP5(x, ...) sk::utils::dumpWithName(TO_PAIR(x)), DUMP4(__VA_ARGS__)
#define DUMP6(x, ...) sk::utils::dumpWithName(TO_PAIR(x)), DUMP5(__VA_ARGS__)
#define DUMP7(x, ...) sk::utils::dumpWithName(TO_PAIR(x)), DUMP6(__VA_ARGS__)
#define DUMP8(x, ...) sk::utils::dumpWithName(TO_PAIR(x)), DUMP7(__VA_ARGS__)

#define GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define DUMP(...)                                                                                \
  do {                                                                                           \
    GET_MACRO(__VA_ARGS__, DUMP8, DUMP7, DUMP6, DUMP5, DUMP4, DUMP3, DUMP2, DUMP1)(__VA_ARGS__); \
    std::cout << "\n";                                                                           \
  } while (0);


// ============================================================================
// Logger
// ============================================================================

#define LOG_SEP ":"

#if SK_LOG_FOR_DEBUG
#define COUT_POSITION \
  "[" << sk::utils::str::basenameWithoutExt(__FILE__) << LOG_SEP << __FUNCTION__ << LOG_SEP << __LINE__ << "]"
#else
#define COUT_POSITION \
  "[" << sk::utils::time::current("%H:%M:%S") << "][" << sk::utils::str::basenameWithoutExt(__FILE__) << "]"
#endif

#define SK_LOG(...)                                                                                \
  do {                                                                                             \
    auto msg__ = sk::utils::colorful_format(__VA_ARGS__);                                          \
    GUARD_LOG;                                                                                     \
    std::cout << ANSI_BLUE_BG << "[DEBUG]" << COUT_POSITION << " " << ANSI_CLEAR << msg__ << "\n"; \
  } while (0);

#define SK_WARN(...)                                                                                 \
  do {                                                                                               \
    auto msg__ = sk::utils::colorful_format(__VA_ARGS__);                                            \
    GUARD_LOG;                                                                                       \
    std::cerr << ANSI_YELLOW_BG << "[ WARN]" << COUT_POSITION << " " << ANSI_CLEAR << msg__ << "\n"; \
  } while (0);

#define SK_ERROR(...)                                                                             \
  do {                                                                                            \
    auto msg__ = sk::utils::colorful_format(__VA_ARGS__);                                         \
    GUARD_LOG;                                                                                    \
    std::cerr << ANSI_RED_BG << "[ERROR]" << COUT_POSITION << " " << ANSI_CLEAR << msg__ << "\n"; \
  } while (0);

#define TODO(msg)                                                                                              \
  do {                                                                                                         \
    GUARD_LOG;                                                                                                 \
    std::cerr << ANSI_YELLOW_BG << "[TODO]" << ANSI_BLUE_BG << COUT_POSITION << ":" << ANSI_PURPLE_BG << (msg) \
              << ANSI_CLEAR << "\n";                                                                           \
  } while (0);

#define FILL_ME() TODO("<== Fill Code Here!!! ")

#define LINE_BREAKER(msg)                                                                         \
  do {                                                                                            \
    GUARD_LOG;                                                                                    \
    std::cout << ANSI_YELLOW_BG << "========== " << (msg) << " ==========" << ANSI_CLEAR << "\n"; \
  } while (0);

#define NEW_LINE()     \
  do {                 \
    GUARD_LOG;         \
    std::cout << "\n"; \
  } while (0);


#endif  // SK_HEADER_ONLY_SKUTILS_H
