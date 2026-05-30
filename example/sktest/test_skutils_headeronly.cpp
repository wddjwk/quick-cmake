/// @file test_skprinter_headeronly.cpp
/// @brief Test that skprinter.h works as a standalone single-header library.
///        Only includes skprinter.h — no other project headers.

#include <cassert>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <stack>
#include <tuple>
#include <variant>
#include <vector>

#include "skutils.h"

struct Animal {
  std::string name;
  int age;

  [[nodiscard]] std::string toString() const {
    std::stringstream ss;
    ss << "{" << name << ELEM_SEP << age << "}";
    return ss.str();
  }
};

enum class Direction { Up = 0, Down = 1, Left = 2, Right = 3 };

#define CHECK_EQ(expected, actual)                                                    \
  do {                                                                                \
    auto _e = (expected);                                                             \
    auto _a = (actual);                                                               \
    if (_e != _a) {                                                                   \
      std::cerr << "[FAIL] " << #expected << " != " << #actual << "\n"                \
                << "  expected: " << _e << "\n  actual:   " << _a << "\n";            \
      ++failures;                                                                     \
    } else {                                                                          \
      std::cout << "[PASS] " << #actual << " == \"" << _e << "\"\n";                  \
      ++passes;                                                                       \
    }                                                                                 \
  } while (0)

int main() {
  int failures = 0;
  int passes = 0;

  LINE_BREAKER("Basic Types");
  CHECK_EQ("42", sk::utils::toString(42));
  CHECK_EQ("3.14", sk::utils::toString(3.14));
  CHECK_EQ("hello", sk::utils::toString("hello"));
  CHECK_EQ("True", sk::utils::toString(true));
  CHECK_EQ("False", sk::utils::toString(false));

  LINE_BREAKER("Containers");
  std::vector<int> vec{1, 2, 3};
  std::list<std::string> lst{"a", "b", "c"};
  std::map<int, std::string> mp{{1, "one"}, {2, "two"}};
  CHECK_EQ(REPLACED_SEP("[1,2,3]"), sk::utils::toString(vec));
  CHECK_EQ(REPLACED_SEP("[a,b,c]"), sk::utils::toString(lst));
  CHECK_EQ(REPLACED_SEP("[{1,one},{2,two}]"), sk::utils::toString(mp));

  LINE_BREAKER("Pair");
  auto pr = std::make_pair(1, std::string("two"));
  CHECK_EQ(REPLACED_SEP("{1,two}"), sk::utils::toString(pr));

  LINE_BREAKER("Serializable");
  Animal cat{"cat", 3};
  CHECK_EQ(REPLACED_SEP("{cat,3}"), sk::utils::toString(cat));

  LINE_BREAKER("Pointer & Nullptr");
  int* pint = new int(99);
  int* pnull = nullptr;
  CHECK_EQ("nullptr", sk::utils::toString(pnull));
  auto pstr = sk::utils::toString(pint);
  // Should contain "=> 99"
  if (pstr.find("=> 99") == std::string::npos) {
    std::cerr << "[FAIL] pointer toString missing '=> 99': " << pstr << "\n";
    ++failures;
  } else {
    std::cout << "[PASS] pointer toString: " << pstr << "\n";
    ++passes;
  }
  delete pint;

  LINE_BREAKER("Smart Pointers");
  auto uptr = std::make_unique<int>(42);
  auto sptr = std::make_shared<std::string>("shared");
  std::unique_ptr<int> uptrNull;
  CHECK_EQ("nullptr", sk::utils::toString(uptrNull));
  auto upstr = sk::utils::toString(uptr);
  if (upstr.find("=> 42") == std::string::npos) {
    std::cerr << "[FAIL] unique_ptr toString missing '=> 42': " << upstr << "\n";
    ++failures;
  } else {
    std::cout << "[PASS] unique_ptr toString: " << upstr << "\n";
    ++passes;
  }
  auto spstr = sk::utils::toString(sptr);
  if (spstr.find("=> shared") == std::string::npos) {
    std::cerr << "[FAIL] shared_ptr toString missing '=> shared': " << spstr << "\n";
    ++failures;
  } else {
    std::cout << "[PASS] shared_ptr toString: " << spstr << "\n";
    ++passes;
  }

  LINE_BREAKER("Optional");
  std::optional<int> optVal = 42;
  std::optional<int> optEmpty;
  CHECK_EQ("optional(42)", sk::utils::toString(optVal));
  CHECK_EQ("nullopt", sk::utils::toString(optEmpty));

  LINE_BREAKER("Tuple");
  auto t2 = std::make_tuple(1, "hello");
  auto t3 = std::make_tuple(1, 2.5, std::string("world"));
  CHECK_EQ(REPLACED_SEP("(1,hello)"), sk::utils::toString(t2));

  LINE_BREAKER("Variant");
  std::variant<int, std::string> v1 = 42;
  std::variant<int, std::string> v2 = std::string("hello");
  CHECK_EQ("variant(42)", sk::utils::toString(v1));
  CHECK_EQ("variant(hello)", sk::utils::toString(v2));

  LINE_BREAKER("Enum");
  Direction d = Direction::Right;
  CHECK_EQ("enum(3)", sk::utils::toString(d));

  LINE_BREAKER("Stack & Queue");
  std::stack<int> stk;
  stk.push(1);
  stk.push(2);
  stk.push(3);
  auto stkStr = sk::utils::toString(stk);
  CHECK_EQ("Stack[3<-2<-1]", stkStr);

  std::queue<int> que;
  que.push(1);
  que.push(2);
  que.push(3);
  auto queStr = sk::utils::toString(que);
  CHECK_EQ("Queue[1<-2<-3]", queStr);

  LINE_BREAKER("Format");
  CHECK_EQ("hello world", sk::utils::format("hello {}", "world"));
  CHECK_EQ("1 + 2 = 3", sk::utils::format("{} + {} = {}", 1, 2, 3));
  // Excess args should not crash
  auto safe = sk::utils::format("only {}", 1, 2, 3);
  CHECK_EQ("only 1", safe);

  LINE_BREAKER("Logger Macros");
  SK_LOG("test log {}", 42);
  SK_WARN("test warn {}", "caution");
  SK_ERROR("test error {}", 3.14);
  TODO("implement this feature");
  std::cout << "[PASS] Logger macros compile and run\n";
  ++passes;

  LINE_BREAKER("DUMP Macro");
  DUMP(vec, mp, cat);
  std::cout << "[PASS] DUMP macro works\n";
  ++passes;

  LINE_BREAKER("Nested Combos");
  std::optional<std::vector<int>> optVec = std::vector<int>{1, 2, 3};
  CHECK_EQ(REPLACED_SEP("optional([1,2,3])"), sk::utils::toString(optVec));

  auto vecOpt = std::vector<std::optional<int>>{{1}, {}, {3}};
  CHECK_EQ(REPLACED_SEP("[optional(1),nullopt,optional(3)]"), sk::utils::toString(vecOpt));

  // Summary
  std::cout << "\n========================================\n";
  std::cout << passes << " passed, " << failures << " failed\n";
  if (failures > 0) {
    std::cerr << "SOME TESTS FAILED!\n";
    return 1;
  }
  std::cout << "ALL TESTS PASSED!\n";
  return 0;
}
