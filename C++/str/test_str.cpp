#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <string_view>
#include <cstring>
#include "str.hpp"

const char* TEST_STRING = "test string";

void RequireEqual(const String& actual, std::string_view expected) {
  REQUIRE(actual.Capacity() >= actual.Size());
  REQUIRE(actual.Size() == expected.size());
  REQUIRE(actual.Length() == expected.size());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    REQUIRE(actual[i] == expected[i]);
  }
}

TEST_CASE("Default constructor + basic Data_access") {
  String s;
  REQUIRE(s.Capacity() == 0);
  REQUIRE(s.Size() == 0);
  REQUIRE(s.Data() == nullptr);
  REQUIRE(s.Empty());
  RequireEqual(s, "");
}

TEST_CASE("Constructors", "C-style-string") {
  SECTION("From non-empty string") {
    const String s = TEST_STRING;
    RequireEqual(s, TEST_STRING);
  }

  SECTION("From empty string") {
    const String s = "";
    REQUIRE(s.Capacity() == 0);
    RequireEqual(s, "");
  }

  SECTION("From string prefix") {
    const String s(TEST_STRING, 4);
    RequireEqual(s, "test");
  }

  SECTION("From empty string prefix") {
    const String s(TEST_STRING, 0);
    REQUIRE(s.Capacity() == 0);
    RequireEqual(s, "");
  }
}

TEST_CASE("Copy Constructor", "[Constructor]") {
  SECTION("Copy empty string") {
    const String empty;
    const auto copy = empty;
    REQUIRE(empty.Size() == 0);
    REQUIRE(empty.Capacity() == 0);
    REQUIRE(empty.Data() == nullptr);
    REQUIRE(copy.Size() == 0);
    REQUIRE(copy.Capacity() == 0);
    REQUIRE(copy.Data() == nullptr);
  }

  SECTION("Copy non-empty string") {
    const String s = TEST_STRING;
    const auto copy = s;
    RequireEqual(copy, TEST_STRING);
    RequireEqual(s, TEST_STRING);
  }
}

TEST_CASE("Operator =") {
  SECTION("Empty to empty") {
    const String empty;
    String s;
    s = empty;
    REQUIRE(empty.Size() == 0);
    REQUIRE(empty.Capacity() == 0);
    REQUIRE(empty.Data() == nullptr);
    REQUIRE(s.Size() == 0);
    REQUIRE(s.Capacity() == 0);
    REQUIRE(s.Data() == nullptr);

    s = s;
    REQUIRE(s.Size() == 0);
    REQUIRE(s.Capacity() == 0);
    REQUIRE(s.Data() == nullptr);
  }

  SECTION("Empty to filled") {
    const String empty;
    String s = TEST_STRING;
    s = empty;
    REQUIRE(empty.Size() == 0);
    REQUIRE(empty.Capacity() == 0);
    REQUIRE(empty.Data() == nullptr);
    REQUIRE(s.Size() == 0);
  }

  SECTION("Filled to empty") {
    const String init = TEST_STRING;
    String s;
    s = init;
    RequireEqual(s, TEST_STRING);
    RequireEqual(init, TEST_STRING);

    s = s;
    RequireEqual(s, TEST_STRING);
    RequireEqual(init, TEST_STRING);
  }

  SECTION("Self-assignment") {
    String s = TEST_STRING;
    s = s;
    RequireEqual(s, TEST_STRING);
  }

  SECTION("Small to large") {
    const String small(10, 'a');
    String large(1000, 'b');
    large = small;
    RequireEqual(large, std::string(10, 'a'));
    RequireEqual(small, std::string(10, 'a'));
  }

  SECTION("Large to small") {
    const String large(1000, 'b');
    String small(10, 'a');
    small = large;
    RequireEqual(large, std::string(1000, 'b'));
    RequireEqual(small, std::string(1000, 'b'));
  }
}

TEST_CASE("Type traits") {
  const String s = TEST_STRING;
  REQUIRE((std::is_same_v<decltype(std::declval<String&>().Data()), const char*>));
  REQUIRE((std::is_same_v<decltype(std::declval<String&>().CStr()), const char*>));

  REQUIRE((std::is_same_v<decltype(std::declval<String&>().operator+=("")), String&>));
  REQUIRE((std::is_same_v<decltype(std::declval<String&>().operator=("")), String&>));
  REQUIRE((std::is_same_v<decltype(std::declval<String&>().operator[](0)), char&>));
  // Just to check const methods.
  REQUIRE(std::as_const(s).Front() == 't');
  REQUIRE(std::as_const(s).Back() == 'g');
  REQUIRE(std::as_const(s)[0] == 't');
  REQUIRE((std::is_same_v<decltype(std::declval<const String&>().Data()), const char*>));
  REQUIRE((std::is_same_v<decltype(std::declval<const String&>().CStr()), const char*>));
  REQUIRE((std::is_same_v<decltype(std::declval<const String&>().operator[](0)), const char&>));
}

TEST_CASE("Data access methods") {
  String s = TEST_STRING;

  SECTION("Front") {
    REQUIRE(s.Front() == 't');
    s.Front() = 'T';
  }

  SECTION("Back") {
    REQUIRE(s.Back() == 'g');
    s.Back() = 'G';
  }

  SECTION("CStr, Data") {
    REQUIRE(s.CStr() == s.Data());
    REQUIRE(strcmp(s.CStr(), TEST_STRING) == 0);
    REQUIRE(s.CStr()[s.Size()] == '\0');  // Null-terminated.
  }

  SECTION("Operator []") {
    for (std::size_t i = 0; i < s.Size(); ++i) {
      REQUIRE(s[i] == TEST_STRING[i]);
    }
  }
}

TEST_CASE("Swap") {
  SECTION("Swap empty") {
    String a;
    String b;
    a.Swap(b);
    RequireEqual(a, "");
    RequireEqual(b, "");
  }

  SECTION("Swap different") {
    String a(TEST_STRING);
    String b;
    ;
    a.Swap(b);
    RequireEqual(a, "");
    RequireEqual(b, TEST_STRING);
  }
}

TEST_CASE("PushBack") {
  String s;
  s.PushBack('a');
  REQUIRE(s == "a");
  REQUIRE(s.Size() == 1);
  REQUIRE(s.Capacity() == 1);
  s.PushBack('a');
  REQUIRE(s == "aa");
  REQUIRE(s.Size() == 2);
  REQUIRE(s.Capacity() == 2);
}

TEST_CASE("Operator +") {
  auto test_data = GENERATE(std::make_tuple(/*lhs=*/"a", /*rhs=*/"b", /*required=*/"ab"),
                            std::make_tuple(/*lhs=*/"aa", /*rhs=*/"b", /*required=*/"aab"),
                            std::make_tuple(/*lhs=*/"a", /*rhs=*/"bb", /*required=*/"abb"),
                            std::make_tuple(/*lhs=*/"aa", /*rhs=*/"bb", /*required=*/"aabb"),
                            std::make_tuple(/*lhs=*/"a", /*rhs=*/"", /*required=*/"a"),
                            std::make_tuple(/*lhs=*/"", /*rhs=*/"b", /*required=*/"b"));

  SECTION("+=") {
    String str = std::get<0>(test_data);
    const String other = std::get<1>(test_data);
    str += other;
    RequireEqual(str, std::get<2>(test_data));
  }

  SECTION("+") {
    const String lhs = std::get<0>(test_data);
    const String rhs = std::get<1>(test_data);
    auto result = std::get<2>(test_data);
    RequireEqual(lhs + rhs, result);
  }
}

TEST_CASE("Clear") {
  String s;
  s.Clear();
  RequireEqual(s, "");

  s = TEST_STRING;
  s.Clear();
  RequireEqual(s, "");
}

TEST_CASE("PopBack") {
  String s = TEST_STRING;
  for (std::size_t i = 0; i < 7; ++i) {
    s.PopBack();
  }
  RequireEqual(s, "test");
}

TEST_CASE("Change size/capacity") {
  SECTION("Resize") {
    String str;
    str.Resize(5, 'a');
    RequireEqual(str, "aaaaa");

    str.Resize(3, 'b');
    RequireEqual(str, "aaa");

    str.Resize(7, 'c');
    RequireEqual(str, "aaacccc");

    str.Resize(0, 'a');
    RequireEqual(str, "");

    str = "aba";
    RequireEqual(str, "aba");
    REQUIRE(str.Capacity() == 3);
  }

  SECTION("Reserve") {
    /*
    Memory locations obtained from the allocator but not available for storing any element are not counted in the
    allocated storage. Note that the null terminator is not an element of the std::basic_string.
    https://en.cppreference.com/w/cpp/string/basic_string/capacity
    */
    String str = TEST_STRING;
    REQUIRE(str.Capacity() == 11);

    str.Reserve(20);
    REQUIRE(str.Capacity() == 20);
    RequireEqual(str, TEST_STRING);

    str.Reserve(1000);
    REQUIRE(str.Capacity() == 1000);
    RequireEqual(str, TEST_STRING);

    str.Reserve(1000000);
    REQUIRE(str.Capacity() == 1000000);
    RequireEqual(str, TEST_STRING);

    str.Reserve(10);
    REQUIRE(str.Capacity() == 1000000);
    RequireEqual(str, TEST_STRING);
  }

  SECTION("ShrinkToFit") {
    String str = TEST_STRING;
    str.Reserve(100);
    REQUIRE(str.Capacity() > str.Size());
    str.ShrinkToFit();
    REQUIRE(str.Capacity() == str.Size());
    RequireEqual(str, TEST_STRING);

    str.Resize(0, 'a');
    RequireEqual(str, "");
    REQUIRE(str.Capacity() > 0);
    str.ShrinkToFit();
    REQUIRE(str.Size() == 0);
    REQUIRE(str.Capacity() == 0);
  }
}

TEST_CASE("Comparisons", "[String]") {
  SECTION("Non-equal") {
    auto test_data = GENERATE(std::make_tuple(/*lhs=*/"a", /*rhs=*/"b"), std::make_tuple(/*lhs=*/"aa", /*rhs=*/"b"),
                              std::make_tuple(/*lhs=*/"a", /*rhs=*/"bb"), std::make_tuple(/*lhs=*/"aa", /*rhs=*/"bb"),
                              std::make_tuple(/*lhs=*/"", /*rhs=*/"a"), std::make_tuple(/*lhs=*/"", /*rhs=*/"b"));

    const String lhs = std::get<0>(test_data);
    const String rhs = std::get<1>(test_data);
    // REQUIRE((lhs <=> rhs) < 0);
    // REQUIRE((rhs <=> lhs) > 0);
    /*The same as:
      REQUIRE_FALSE(lhs == rhs);
      REQUIRE(lhs <= rhs);
      REQUIRE_FALSE(lhs >= rhs);
      REQUIRE(lhs != rhs);
      REQUIRE(lhs < rhs);
      REQUIRE_FALSE(lhs > rhs);
     */
  }

  SECTION("Equal") {
    auto test_data = GENERATE(std::make_tuple(/*lhs=*/"a", /*rhs=*/"a"), std::make_tuple(/*lhs=*/"aa", /*rhs=*/"aa"),
                              std::make_tuple(/*lhs=*/"b", /*rhs=*/"b"), std::make_tuple(/*lhs=*/"bb", /*rhs=*/"bb"),
                              std::make_tuple(/*lhs=*/"", /*rhs=*/""));

    const String lhs = std::get<0>(test_data);
    const String rhs = std::get<1>(test_data);
    // REQUIRE((lhs <=> rhs) == 0);
    /*The same as:
      REQUIRE(lhs == rhs);
      REQUIRE(lhs <= rhs);
      REQUIRE(lhs >= rhs);
      REQUIRE_FALSE(lhs != rhs);
      REQUIRE_FALSE(lhs < rhs);
      REQUIRE_FALSE(lhs > rhs);
     */
  }
}

TEST_CASE("Output", "[String]") {
  auto oss = std::ostringstream();
  oss << String(TEST_STRING) << ' ' << String() << ' ' << String(TEST_STRING, 4);
  REQUIRE(oss.str() == "test string  test");
}
