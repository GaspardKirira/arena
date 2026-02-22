#include <arena/arena.hpp>
#include <iostream>

struct Temp
{
  int x;
  Temp(int v) : x(v) {}
};

int main()
{
  arena::Arena a(4096);

  std::cout << "Before: " << a.used() << "\n";

  {
    arena::Arena::Scope scope(a);

    Temp *t1 = a.make<Temp>(10);
    Temp *t2 = a.make<Temp>(20);

    std::cout << "Inside scope: " << a.used() << "\n";
    std::cout << t1->x << ", " << t2->x << "\n";
  }

  std::cout << "After scope: " << a.used() << "\n";

  return 0;
}
