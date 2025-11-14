#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <memory>
#include <vector>
#include <utility>

#include "unique_pointer.hpp"

TEST_CASE("Type traits") {
  REQUIRE_FALSE(std::is_copy_constructible_v<UniquePtr<int>>);
  REQUIRE_FALSE(std::is_copy_assignable_v<UniquePtr<int>>);
  REQUIRE(std::is_nothrow_move_assignable_v<UniquePtr<int>>);
  REQUIRE(std::is_nothrow_move_constructible_v<UniquePtr<int>>);
  REQUIRE(std::is_default_constructible_v<UniquePtr<int>>);
  struct Base {};
  struct Derived : Base {};
  REQUIRE(std::is_convertible_v<UniquePtr<Derived>, UniquePtr<Base>>);
}

TEST_CASE("Constructor") {
  SECTION("Default Constructor") {
    const UniquePtr<int> u_ptr;
    REQUIRE_FALSE(u_ptr);
    REQUIRE(u_ptr.Get() == nullptr);
  }

  SECTION("Constructor from raw pointer") {
    SECTION("nullptr") {
      const UniquePtr<int> u_ptr(nullptr);
      REQUIRE_FALSE(u_ptr);
      REQUIRE(u_ptr.Get() == nullptr);
    }

    SECTION("int*") {
      auto ptr = new int;
      const UniquePtr<int> u_ptr(ptr);
      REQUIRE(u_ptr);
      REQUIRE(u_ptr.Get() == ptr);
    }
  }

  SECTION("Move Constructor") {
    auto ptr = new int;
    UniquePtr<int> u_ptr(ptr);
    const UniquePtr<int> other_u_ptr(std::move(u_ptr));
    REQUIRE(other_u_ptr);
    REQUIRE_FALSE(u_ptr);
    REQUIRE(u_ptr.Get() == nullptr);
    REQUIRE(other_u_ptr.Get() == ptr);
    REQUIRE(std::is_nothrow_move_constructible_v<UniquePtr<int>>);
    REQUIRE(!std::is_copy_constructible_v<UniquePtr<int>>);
  }
}

TEST_CASE("Operator =") {
  auto ptr = new int;
  UniquePtr<int> u_ptr(ptr);
  UniquePtr<int> other_u_ptr;

  other_u_ptr = std::move(u_ptr);
  REQUIRE(other_u_ptr);
  REQUIRE_FALSE(u_ptr);
  REQUIRE(u_ptr.Get() == nullptr);
  REQUIRE(other_u_ptr.Get() == ptr);

  auto ptr2 = new int;
  other_u_ptr = UniquePtr<int>(ptr2);
  REQUIRE(other_u_ptr);
  REQUIRE(other_u_ptr.Get() == ptr2);
}

TEST_CASE("Release") {
  SECTION("Empty") {
    UniquePtr<int> u_ptr;
    REQUIRE(u_ptr.Release() == nullptr);
  }

  SECTION("Not Empty") {
    auto ptr = new int;
    UniquePtr<int> u_ptr(ptr);
    REQUIRE(u_ptr.Release() == ptr);
    REQUIRE(u_ptr.Get() == nullptr);
    REQUIRE_FALSE(u_ptr);
    delete ptr;
  }
}

TEST_CASE("Reset") {
  SECTION("Empty") {
    auto ptr = new int;
    UniquePtr<int> u_ptr;

    u_ptr.Reset();
    REQUIRE_FALSE(u_ptr);
    REQUIRE(u_ptr.Get() == nullptr);

    u_ptr.Reset(ptr);
    REQUIRE(u_ptr);
    REQUIRE(u_ptr.Get() == ptr);
  }

  SECTION("Not Empty") {
    auto ptr1 = new int;
    auto ptr2 = new int;
    UniquePtr<int> u_ptr(ptr1);

    u_ptr.Reset(ptr2);
    REQUIRE(u_ptr);
    REQUIRE(u_ptr.Get() == ptr2);

    u_ptr.Reset();
    REQUIRE(!u_ptr);
    REQUIRE(u_ptr.Get() == nullptr);
  }
}

TEST_CASE("Swap") {
  auto ptr1 = new int;
  auto ptr2 = new int;
  UniquePtr<int> first_u_ptr;
  UniquePtr<int> second_u_ptr(ptr1);
  UniquePtr<int> third_u_ptr(ptr2);

  SECTION("Self") {
    first_u_ptr.Swap(first_u_ptr);
    REQUIRE(!first_u_ptr);
    REQUIRE(first_u_ptr.Get() == nullptr);

    second_u_ptr.Swap(second_u_ptr);
    REQUIRE(second_u_ptr);
    REQUIRE(second_u_ptr.Get() == ptr1);
  }

  SECTION("Other") {
    first_u_ptr.Swap(second_u_ptr);
    REQUIRE(first_u_ptr);
    REQUIRE(first_u_ptr.Get() == ptr1);
    REQUIRE(!second_u_ptr);
    REQUIRE(second_u_ptr.Get() == nullptr);

    second_u_ptr.Swap(third_u_ptr);
    REQUIRE(second_u_ptr);
    REQUIRE(second_u_ptr.Get() == ptr2);
    REQUIRE(!third_u_ptr);
    REQUIRE(third_u_ptr.Get() == nullptr);

    first_u_ptr.Swap(second_u_ptr);
    REQUIRE(first_u_ptr);
    REQUIRE(first_u_ptr.Get() == ptr2);
    REQUIRE(second_u_ptr);
    REQUIRE(second_u_ptr.Get() == ptr1);
  }
}

TEST_CASE("Data access") {
  const UniquePtr<std::pair<int, double>> u_ptr(new std::pair<int, double>{});
  REQUIRE(u_ptr->first == 0);
  REQUIRE((*u_ptr).second == 0.0);

  u_ptr->first = 1;
  (*u_ptr).second = 1.5;
  REQUIRE(u_ptr->first == 1);
  REQUIRE((*u_ptr).second == 1.5);

  u_ptr.Get()->first = 0;
  u_ptr.Get()->second = 0.5;
  REQUIRE(u_ptr->first == 0);
  REQUIRE((*u_ptr).second == 0.5);
}

TEST_CASE("MakeUnique") {
  SECTION("Trivial type") {
    const auto ptr = MakeUnique<int>(0);
    REQUIRE(*ptr == 0);
  }

  SECTION("Constructor with parameters") {
    const auto ptr = MakeUnique<std::pair<int, int>>(1, 1);
    REQUIRE(ptr->first == 1);
    REQUIRE(ptr->second == 1);
  }
}
