#include <arena/arena.hpp>

#include <cassert>
#include <cstdint>
#include <string>

namespace
{
  struct Foo
  {
    int a;
    std::string s;

    Foo(int x, std::string y) : a(x), s(std::move(y)) {}
  };

  static void test_basic_alloc()
  {
    arena::Arena a(1024);

    auto *p1 = static_cast<std::uint8_t *>(a.allocate(1, 1));
    assert(a.owns(p1));
    assert(a.used() >= 1);

    auto *p2 = static_cast<std::uint64_t *>(a.allocate(sizeof(std::uint64_t), alignof(std::uint64_t)));
    assert(a.owns(p2));
    assert((reinterpret_cast<std::uintptr_t>(p2) % alignof(std::uint64_t)) == 0);
  }

  static void test_make_and_scope()
  {
    arena::Arena a(4096);

    const std::size_t before = a.used();
    {
      arena::Arena::Scope scope(a);

      Foo *f = a.make<Foo>(42, "hello");
      assert(f->a == 42);
      assert(f->s == "hello");

      int *arr = a.make_array<int>(100);
      assert(arr != nullptr);
      arr[0] = 7;
      arr[99] = 9;
      assert(arr[0] == 7);
      assert(arr[99] == 9);

      assert(a.used() > before);
    }

    assert(a.used() == before);
  }

  static void test_reset()
  {
    arena::Arena a(256);
    (void)a.allocate(64);
    assert(a.used() > 0);
    a.reset();
    assert(a.used() == 0);
  }
}

int main()
{
  test_basic_alloc();
  test_make_and_scope();
  test_reset();
  return 0;
}
