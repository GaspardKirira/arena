#include <arena/arena.hpp>
#include <iostream>
#include <cstring>

int main()
{
  arena::Arena scratch(1024);

  const char *text = "temporary parsing data";

  {
    arena::Arena::Scope scope(scratch);

    char *buffer = static_cast<char *>(scratch.allocate(64));
    std::memcpy(buffer, text, std::strlen(text) + 1);

    std::cout << buffer << "\n";
  }

  // buffer invalid here
  std::cout << "Scratch reset automatically\n";

  return 0;
}
