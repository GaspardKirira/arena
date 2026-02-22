#include <arena/arena.hpp>
#include <iostream>
#include <string>

struct User
{
  int id;
  std::string name;

  User(int i, std::string n)
      : id(i), name(std::move(n)) {}
};

int main()
{
  arena::Arena a(8192);

  User *u = a.make<User>(1, "Alice");
  std::cout << u->id << " " << u->name << "\n";

  int *numbers = a.make_array<int>(5);
  for (int i = 0; i < 5; ++i)
    numbers[i] = i * 10;

  for (int i = 0; i < 5; ++i)
    std::cout << numbers[i] << " ";

  std::cout << "\nUsed: " << a.used() << "\n";
  return 0;
}
