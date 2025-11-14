#define CATCH_CONFIG_MAIN

#include <catch.hpp>

#include <vector>
#include <utility>

#include "shared_pointer.hpp"

TEST_CASE("Shared type traits", "[SharedPtr]") {
  REQUIRE(std::is_copy_constructible_v<SharedPtr<int>>);
  REQUIRE(std::is_copy_assignable_v<SharedPtr<int>>);
  REQUIRE(std::is_nothrow_move_assignable_v<SharedPtr<int>>);
  REQUIRE(std::is_nothrow_move_constructible_v<SharedPtr<int>>);
  REQUIRE(std::is_default_constructible_v<SharedPtr<int>>);
  struct Base {};
  struct Derived : Base {};
  REQUIRE(std::is_convertible_v<SharedPtr<Derived>, SharedPtr<Base>>);
}

TEST_CASE("Shared Constructor", "[SharedPtr]") {
  SECTION("Default") {
    const SharedPtr<int> shared_ptr;
    REQUIRE_FALSE(shared_ptr);
    REQUIRE(shared_ptr.Get() == nullptr);
    REQUIRE(shared_ptr.UseCount() == 0);
  }

  SECTION("Raw pointer", "[SharedPtr]") {
    SECTION("nullptr") {
      SharedPtr<int> shared_ptr(nullptr);
      REQUIRE_FALSE(shared_ptr);
      REQUIRE(shared_ptr.Get() == nullptr);
      REQUIRE(shared_ptr.UseCount() == 0);
    }

    SECTION("int*") {
      auto raw_ptr = new int;
      const SharedPtr<int> shared_ptr(raw_ptr);
      REQUIRE(shared_ptr);
      REQUIRE(shared_ptr.Get() == raw_ptr);
      REQUIRE(shared_ptr.UseCount() == 1);
    }
  }

  SECTION("Move", "[SharedPtr]") {
    auto raw_ptr = new int;
    SharedPtr<int> first_shared_ptr(raw_ptr);
    SharedPtr<int> second_shared_ptr(std::move(first_shared_ptr));
    REQUIRE(second_shared_ptr);
    REQUIRE(second_shared_ptr.Get() == raw_ptr);
    REQUIRE(second_shared_ptr.UseCount() == 1);

    REQUIRE_FALSE(first_shared_ptr);
    REQUIRE(first_shared_ptr.Get() == nullptr);
    REQUIRE(first_shared_ptr.UseCount() == 0);
  }

  SECTION("Copy", "[SharedPtr]") {
    auto raw_ptr = new int;
    SharedPtr<int> first_shared_ptr(raw_ptr);
    SharedPtr<int> second_shared_ptr(first_shared_ptr);

    REQUIRE(first_shared_ptr);
    REQUIRE(first_shared_ptr.Get() == raw_ptr);
    REQUIRE(first_shared_ptr.UseCount() == 2);

    REQUIRE(second_shared_ptr);
    REQUIRE(second_shared_ptr.Get() == raw_ptr);
    REQUIRE(second_shared_ptr.UseCount() == 2);
  }
}

TEST_CASE("Operator=", "[SharedPtr]") {
  SECTION("Self") {
    auto raw_ptr = new int;
    SharedPtr<int> first_shared_ptr(raw_ptr);
    first_shared_ptr = first_shared_ptr;
    REQUIRE(first_shared_ptr);
    REQUIRE(first_shared_ptr.Get() == raw_ptr);
    REQUIRE(first_shared_ptr.UseCount() == 1);
  }

  SECTION("Copy") {
    auto raw_ptr = new int;
    SharedPtr<int> first_shared_ptr(raw_ptr);
    SharedPtr<int> second_shared_ptr;
    second_shared_ptr = first_shared_ptr;
    REQUIRE(first_shared_ptr);
    REQUIRE(first_shared_ptr.Get() == raw_ptr);
    REQUIRE(first_shared_ptr.UseCount() == 2);

    REQUIRE(second_shared_ptr);
    REQUIRE(second_shared_ptr.Get() == raw_ptr);
    REQUIRE(second_shared_ptr.UseCount() == 2);
  }

  SECTION("Copy is independent") {
    auto raw_ptr = new int;
    SharedPtr<int> first_shared_ptr(raw_ptr);
    SharedPtr<int> second_shared_ptr;
    second_shared_ptr = first_shared_ptr;
    second_shared_ptr = SharedPtr<int>();
    REQUIRE(first_shared_ptr);
    REQUIRE(first_shared_ptr.Get() == raw_ptr);
    REQUIRE(first_shared_ptr.UseCount() == 1);

    REQUIRE_FALSE(second_shared_ptr);
    REQUIRE(second_shared_ptr.Get() == nullptr);
    REQUIRE(second_shared_ptr.UseCount() == 0);
  }

  SECTION("Reassignment") {
    auto raw_ptr = new int;
    SharedPtr<int> first_shared_ptr(raw_ptr);
    auto other_raw_ptr = new int;
    first_shared_ptr = SharedPtr<int>(other_raw_ptr);            ///without int
    REQUIRE(first_shared_ptr);
    REQUIRE(first_shared_ptr.Get() == other_raw_ptr);
    REQUIRE(first_shared_ptr.UseCount() == 1);
  }

  SECTION("Move") {
    auto raw_ptr = new int;
    SharedPtr<int> first_shared_ptr(raw_ptr);
    SharedPtr<int> second_shared_ptr;
    second_shared_ptr = std::move(first_shared_ptr);
    REQUIRE_FALSE(first_shared_ptr);
    REQUIRE(first_shared_ptr.Get() == nullptr);
    REQUIRE(first_shared_ptr.UseCount() == 0);

    REQUIRE(second_shared_ptr);
    REQUIRE(second_shared_ptr.Get() == raw_ptr);
    REQUIRE(second_shared_ptr.UseCount() == 1);
  }

  REQUIRE(std::is_nothrow_move_assignable_v<SharedPtr<int>>);
}

TEST_CASE("UseCount", "[SharedPtr]") {
  SharedPtr<int> a;
  const SharedPtr<int> b(new int(6));

  REQUIRE(a.UseCount() == 0);
  REQUIRE(b.UseCount() == 1);

  a = b;
  REQUIRE(a.UseCount() == 2);
  REQUIRE(b.UseCount() == 2);

  {  // Copy/move
    SharedPtr<int> c(a);
    REQUIRE(a.UseCount() == 3);
    REQUIRE(b.UseCount() == 3);
    REQUIRE(c.UseCount() == 3);

    const SharedPtr<int> d(std::move(c));
    REQUIRE(a.UseCount() == 3);
    REQUIRE(b.UseCount() == 3);
    REQUIRE(c.UseCount() == 0);
    REQUIRE(d.UseCount() == 3);
  }  // Release in destructor.

  REQUIRE(a.UseCount() == 2);
  REQUIRE(b.UseCount() == 2);

  a = nullptr;
  REQUIRE(a.UseCount() == 0);
  REQUIRE(b.UseCount() == 1);
}

TEST_CASE("Swap", "[SharedPtr]") {
  auto ptr1 = new int;
  auto ptr2 = new int;
  SharedPtr<int> a;
  SharedPtr<int> b(ptr1);
  SharedPtr<int> c(ptr2);
  const SharedPtr<int> d(c);

  SECTION("Empty with non-empty") {
    auto raw_ptr = new int;

    SharedPtr<int> first_shared_ptr(raw_ptr);
    SharedPtr<int> second_shared_ptr;
    first_shared_ptr.Swap(second_shared_ptr);
    REQUIRE(first_shared_ptr.Get() == nullptr);
    REQUIRE(first_shared_ptr.UseCount() == 0);
    REQUIRE(second_shared_ptr.Get() == raw_ptr);
    REQUIRE(second_shared_ptr.UseCount() == 1);
  }

  SECTION("Self") {
    auto raw_ptr = new int;

    SharedPtr<int> shared_ptr(raw_ptr);
    shared_ptr.Swap(shared_ptr);

    REQUIRE(shared_ptr.Get() == raw_ptr);
    REQUIRE(shared_ptr.UseCount() == 1);
  }

  SECTION("Other shared pointers not affected") {
    auto first_raw_ptr = new int;
    auto second_raw_ptr = new int;

    SharedPtr<int> first_shared_ptr(first_raw_ptr);
    SharedPtr<int> second_shared_ptr(second_raw_ptr);
    SharedPtr<int> third_shared_ptr(first_shared_ptr);

    REQUIRE(third_shared_ptr.Get() == first_raw_ptr);
    REQUIRE(third_shared_ptr.UseCount() == 2);
  }
}

TEST_CASE("Reset", "[SharedPtr]") {
  {  // reset empty
    SharedPtr<int> a;
    a.Reset();
    REQUIRE(a.UseCount() == 0);

    a.Reset(new int);
    REQUIRE(a.UseCount() == 1);

    a.Reset();
    REQUIRE(a.UseCount() == 0);
  }

  {  // reset shared
    auto ptr1 = new int(5);
    SharedPtr<int> a(ptr1);
    SharedPtr<int> b = a;

    b.Reset();
    REQUIRE(a.UseCount() == 1);
    REQUIRE(a.Get() == ptr1);
    REQUIRE(b.UseCount() == 0);
    REQUIRE(b.Get() == nullptr);

    b = a;
    auto ptr2 = new int(7);
    a.Reset(ptr2);
    REQUIRE(a.UseCount() == 1);
    REQUIRE(a.Get() == ptr2);
    REQUIRE(b.UseCount() == 1);
    REQUIRE(b.Get() == ptr1);
  }
}

TEST_CASE("Data access", "[SharedPtr]") {
  SECTION("operator*") {
    const SharedPtr<int> a(new int(19));
    REQUIRE(*a == 19);

    *a = 11;
    REQUIRE(*a == 11);

    *a.Get() = -11;
    REQUIRE(*a == -11);
  }

  SECTION("operator->") {
    auto ptr = new int(11);
    const SharedPtr<SharedPtr<int>> a(new SharedPtr<int>(ptr));

    REQUIRE(a->UseCount() == 1);
    REQUIRE(a->Get() == ptr);

    a->Reset();
    REQUIRE(a->UseCount() == 0);
    REQUIRE(a->Get() == nullptr);
  }
}

TEST_CASE("Weak type traits", "[WeakPtr]") {
  REQUIRE(std::is_copy_constructible_v<WeakPtr<int>>);
  REQUIRE(std::is_copy_assignable_v<WeakPtr<int>>);
  REQUIRE(std::is_nothrow_move_assignable_v<WeakPtr<int>>);
  REQUIRE(std::is_nothrow_move_constructible_v<WeakPtr<int>>);
  REQUIRE(std::is_default_constructible_v<WeakPtr<int>>);
  struct Base {};
  struct Derived : Base {};
  REQUIRE(std::is_convertible_v<WeakPtr<Derived>, WeakPtr<Base>>);
}

TEST_CASE("WeakConstructor", "[WeakPtr]") {
  const SharedPtr<int> shared(new int);
  const WeakPtr<int> a;
  WeakPtr<int> b(a);
  const WeakPtr<int> c(shared);
  const WeakPtr<int> d(std::move(b));
}

TEST_CASE("UseCount+Expired", "[WeakPtr]") {
  {
    const WeakPtr<int> a;
    WeakPtr<int> b(a);
    const WeakPtr<int> c(std::move(b));

    REQUIRE(a.UseCount() == 0);
    REQUIRE(b.UseCount() == 0);
    REQUIRE(c.UseCount() == 0);
    REQUIRE(a.Expired());
    REQUIRE(b.Expired());
    REQUIRE(c.Expired());
  }

  WeakPtr<int> a;
  {
    SharedPtr<int> ptr1(new int(1));
    const auto ptr2 = ptr1;
    auto ptr3 = ptr2;
    a = ptr2;
    const WeakPtr<int> b = ptr3;

    REQUIRE(a.UseCount() == 3);
    REQUIRE(b.UseCount() == 3);
    REQUIRE(ptr1.UseCount() == 3);
    REQUIRE(ptr2.UseCount() == 3);
    REQUIRE(ptr3.UseCount() == 3);
    REQUIRE_FALSE(a.Expired());
    REQUIRE_FALSE(b.Expired());

    ptr1.Reset();
    ptr3.Reset();
    REQUIRE(a.UseCount() == 1);
    REQUIRE(b.UseCount() == 1);
    REQUIRE(ptr1.UseCount() == 0);
    REQUIRE(ptr2.UseCount() == 1);
    REQUIRE(ptr3.UseCount() == 0);
    REQUIRE_FALSE(a.Expired());
    REQUIRE_FALSE(b.Expired());
  }
  REQUIRE(a.Expired());
}

TEST_CASE("Weak Reset", "[WeakPtr]") {
  WeakPtr<int> a;
  a.Reset();
  REQUIRE(a.UseCount() == 0);
  REQUIRE(a.Expired());

  const SharedPtr<int> ptr(new int);
  WeakPtr<int> b = ptr;
  a = b;
  b.Reset();
  REQUIRE(ptr.UseCount() == 1);
  REQUIRE(a.UseCount() == 1);
  REQUIRE(b.UseCount() == 0);
  REQUIRE(b.Expired());
  REQUIRE_FALSE(a.Expired());

  SharedPtr<int> empty_ptr;
  const WeakPtr<int> c = empty_ptr;
  REQUIRE(c.Expired());

  empty_ptr.Reset(new int);
  REQUIRE(c.Expired());
}

TEST_CASE("Lock", "[WeakPtr]") {
  WeakPtr<int> weak_ptr;
  REQUIRE(weak_ptr.Lock().Get() == nullptr);

  SECTION("Stored pointers are independent") {
    auto raw_ptr = new int;
    const SharedPtr<int> shared_ptr(raw_ptr);
    WeakPtr<int> other_weak_ptr = shared_ptr;
    weak_ptr = other_weak_ptr;
    other_weak_ptr.Reset();
    REQUIRE(other_weak_ptr.Lock().Get() == nullptr);
    REQUIRE(weak_ptr.Lock().Get() == raw_ptr);
  }

  SECTION("Shared independent from weak") {
    auto raw_ptr = new int;
    SharedPtr<int> ptr(raw_ptr);
    const WeakPtr<int> b = ptr;
    auto ptr_tmp = b.Lock();

    ptr.Reset();
    REQUIRE(ptr_tmp.Get() == raw_ptr);
    REQUIRE(ptr_tmp.UseCount() == 1);
  }
}

TEST_CASE("Construct from weak", "[SharedPtr]]") {
  SECTION("Alive weak pointer") {
    const auto ptr = new int;
    const SharedPtr<int> init(ptr);
    const WeakPtr<int> weak(init);
    const SharedPtr<int> a(weak);
    const SharedPtr<int> b(weak);

    REQUIRE(!weak.Expired());
    REQUIRE(weak.UseCount() == 3);
    REQUIRE(init.UseCount() == 3);
    REQUIRE(a.UseCount() == 3);
    REQUIRE(b.UseCount() == 3);

    SECTION("Empty weak pointer") {
      const WeakPtr<int> w_ptr;
      REQUIRE_THROWS_AS(SharedPtr<int>(w_ptr), std::bad_weak_ptr);
    }

    SECTION("Expired weak pointer") {
      auto x = SharedPtr<int>(new int(0));
      const WeakPtr<int> y(x);
      x.Reset();
      REQUIRE(y.Expired());
      REQUIRE_THROWS_AS(SharedPtr<int>(y), std::bad_weak_ptr);
    }
  }
}

TEST_CASE("MakeShared", "[SharedPtr]") {
  SECTION("Trivial type") {
    const auto ptr = MakeShared<int>(0);
    REQUIRE(*ptr == 0);
  }

  SECTION("Constructor with parameters") {
    const auto ptr = MakeShared<std::pair<int, int>>(1, 1);
    REQUIRE(ptr->first == 1);
    REQUIRE(ptr->second == 1);
  }
}
