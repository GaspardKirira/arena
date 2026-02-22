#include <arena/arena.hpp>
#include <iostream>

int main()
{
  arena::Arena a(1024);

  void *p1 = a.allocate(32);
  void *p2 = a.allocate(64);

  std::cout << "Used: " << a.used() << " bytes\n";

  a.reset();

  std::cout << "After reset: " << a.used() << " bytes\n";
  return 0;
}
