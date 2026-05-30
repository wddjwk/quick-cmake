#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

#include "skutils/logger.h"
#include "skutils/printer.h"
#include "skutils/string_utils.h"
#include "skutils/test.h"

struct People {};

struct Person {
  int age;                // NOLINT
  char sex;               // NOLINT
  std::string_view name;  // NOLINT

  [[nodiscard]] std::string toString() const {
    std::stringstream ss;
    ss << "[" << name << ELEM_SEP << age << ELEM_SEP << (sex == 'm' ? "male" : "female") << "]";
    return ss.str();
  }
};

enum class Color { Red = 0, Green = 1, Blue = 2 };
enum UnscopedColor { UC_Red = 10, UC_Green = 20, UC_Blue = 30 };

int main() {
  std::vector<std::vector<int>> vc{{1, 2}, {3, 4}};
  std::map<int, std::vector<int>> mp{{1, {1, 2}}, {2, {2, 3}}};
  std::list<int> lst{1, 2, 3, 4, 5};

  LINE_BREAKER("test utils test");
  ASSERT_STR_EQUAL(REPLACED_SEP("[[1,2],[3,4]]"), sk::utils::toString(vc));
  ASSERT_STR_EQUAL(REPLACED_SEP("[{1,[1,2]},{2,[2,3]}]"), sk::utils::toString(mp));
  ASSERT_STR_EQUAL(REPLACED_SEP("[1,2,3,4,5]"), sk::utils::toString(lst));

  LINE_BREAKER("printer test");
  DUMP(sk::utils::toString(vc));
  sk::utils::print("{}", mp);
  Person person{.age = 18, .sex = 'm', .name = "shuaikai"};
  DUMP(sk::utils::toString(person));
  sk::utils::dump(sk::utils::toString(vc), sk::utils::toString(mp));

  LINE_BREAKER("DUMP test");
  DUMP(vc, mp, person, true);

  LINE_BREAKER("Pointer Test");
  std::vector<int>* pvc = new std::vector<int>{1, 2, 3};
  People* pPeople = new People();
  Person* pPerson = new Person{.age = 18, .sex = 'm', .name = "shuaikai"};
  const char* pstr = "hello";
  const char* pstrarr[] = {"hi", "world"};
  int* pint = new int(99);
  auto lambda_func = [](int a) { return a; };

  DUMP(*pvc, pvc, &pvc);
  DUMP(*pPerson, pPerson, &pPerson);
  DUMP(pPeople, &pPeople);
  DUMP(*pstr, pstr, &pstr);
  DUMP(**pstrarr, *pstrarr, pstrarr, &pstrarr)
  DUMP(*pint, pint, &pint);
  DUMP(*lambda_func, lambda_func, &lambda_func);

  // Bug fix #1: nullptr safety
  LINE_BREAKER("Nullptr Test");
  int* pnull = nullptr;
  Person* pPersonNull = nullptr;
  DUMP(pnull, pPersonNull);
  ASSERT_STR_EQUAL("nullptr", sk::utils::toString(pnull));
  ASSERT_STR_EQUAL("nullptr", sk::utils::toString(pPersonNull));

  // Bug fix #2: format() with excess args (should not crash)
  LINE_BREAKER("Format Safety Test");
  auto fmtResult = sk::utils::format("only one: {}", 42, 99, "extra");
  DUMP(fmtResult);
  auto fmtNoSlot = sk::utils::format("no slots at all", 1, 2, 3);
  DUMP(fmtNoSlot);

  // Feature #4: std::optional
  LINE_BREAKER("Optional Test");
  std::optional<int> optVal = 42;
  std::optional<int> optEmpty;
  std::optional<std::vector<int>> optVec = std::vector<int>{1, 2, 3};
  std::optional<std::string> optStr = "hello";
  DUMP(optVal, optEmpty, optVec, optStr);
  ASSERT_STR_EQUAL("optional(42)", sk::utils::toString(optVal));
  ASSERT_STR_EQUAL("nullopt", sk::utils::toString(optEmpty));
  ASSERT_STR_EQUAL(REPLACED_SEP("optional([1,2,3])"), sk::utils::toString(optVec));
  ASSERT_STR_EQUAL("optional(hello)", sk::utils::toString(optStr));

  // Feature #5: smart pointers
  LINE_BREAKER("Smart Pointer Test");
  auto uptr = std::make_unique<int>(42);
  auto sptr = std::make_shared<std::vector<int>>(std::vector<int>{1, 2, 3});
  std::unique_ptr<int> uptrNull;
  std::shared_ptr<Person> sptrPerson = std::make_shared<Person>(Person{.age = 25, .sex = 'f', .name = "alice"});
  // DUMP macro copies values, so use toString() directly for unique_ptr
  sk::utils::dump(sk::utils::toString(uptr), sk::utils::toString(sptr),
                  sk::utils::toString(uptrNull), sk::utils::toString(sptrPerson));
  ASSERT_STR_EQUAL("nullptr", sk::utils::toString(uptrNull));
  DUMP(sptr, sptrPerson);

  // Feature #6: std::tuple
  LINE_BREAKER("Tuple Test");
  auto t2 = std::make_tuple(1, "hello");
  auto t3 = std::make_tuple(1, 2.5, std::string("world"));
  auto t1 = std::make_tuple(42);
  DUMP(t2, t3, t1);
  ASSERT_STR_EQUAL(REPLACED_SEP("(1,hello)"), sk::utils::toString(t2));
  ASSERT_STR_EQUAL("(42)", sk::utils::toString(t1));

  // Feature #7: std::variant
  LINE_BREAKER("Variant Test");
  std::variant<int, std::string, double> v1 = 42;
  std::variant<int, std::string, double> v2 = std::string("hello");
  std::variant<int, std::string, double> v3 = 3.14;
  DUMP(v1, v2, v3);
  ASSERT_STR_EQUAL("variant(42)", sk::utils::toString(v1));
  ASSERT_STR_EQUAL("variant(hello)", sk::utils::toString(v2));
  ASSERT_STR_EQUAL("variant(3.14)", sk::utils::toString(v3));

  // Feature #8: enum
  LINE_BREAKER("Enum Test");
  Color c = Color::Blue;
  DUMP(c);
  ASSERT_STR_EQUAL("enum(2)", sk::utils::toString(c));
  // Unscoped enums go through StreamOutable (prints as int)
  UnscopedColor uc = UC_Green;
  DUMP(uc);
  ASSERT_STR_EQUAL("20", sk::utils::toString(uc));

  // Nested combinations
  LINE_BREAKER("Nested Combo Test");
  std::optional<std::tuple<int, std::string>> optTuple = std::make_tuple(1, std::string("nested"));
  auto vecOfOpt = std::vector<std::optional<int>>{{1}, {}, {3}};
  DUMP(optTuple, vecOfOpt);

  delete pvc;
  delete pPeople;
  delete pPerson;
  delete pint;

  return ASSERT_ALL_PASSED();
}
