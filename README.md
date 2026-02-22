# arena

Fast bump-pointer arena allocator for modern C++.

arena provides ultra-fast linear memory allocation for temporary object
lifetimes.

It is designed for performance-critical systems where allocations are
frequent and deallocation can happen in bulk.

Unlike general-purpose allocators, this arena:

-   Allocates in O(1)
-   Does not perform per-object free
-   Supports mark and rewind
-   Is fully header-only
-   Has zero dependencies

Perfect for:

-   Scratch memory
-   Game engine frame allocators
-   Temporary parsing buffers
-   AST construction
-   ECS systems
-   High-performance pipelines

## Why arena?

`new` and `delete` are powerful but expensive when used heavily.

This arena uses a bump-pointer strategy:

-   Memory grows linearly
-   No fragmentation
-   No free list
-   No allocator bookkeeping overhead
-   Constant-time reset

If your allocations share the same lifetime, this is dramatically
faster.

## Installation

### Using Vix Registry

``` bash
vix add gaspardkirira/arena
vix deps
```

### Manual

Clone the repository:

``` bash
git clone https://github.com/GaspardKirira/arena.git
```

Add the `include/` directory to your project.

## Quick Example

``` cpp
#include <arena/arena.hpp>
#include <iostream>

int main()
{
  arena::Arena a(1024);

  int* x = static_cast<int*>(a.allocate(sizeof(int)));
  *x = 42;

  std::cout << *x << "\n";

  a.reset(); // frees everything in O(1)
}
```

## Object Construction

``` cpp
#include <arena/arena.hpp>
#include <string>
#include <iostream>

struct User
{
  int id;
  std::string name;

  User(int i, std::string n)
    : id(i), name(std::move(n)) {}
};

int main()
{
  arena::Arena a(4096);

  User* u = a.make<User>(1, "Alice");
  std::cout << u->name << "\n";
}

// Note: destructors are not called automatically.
```

## Scoped Temporary Allocations

Use RAII scopes to automatically rewind memory.

``` cpp
#include <arena/arena.hpp>
#include <iostream>

int main()
{
  arena::Arena a(4096);

  {
    arena::Arena::Scope scope(a);

    int* arr = a.make_array<int>(100);
    arr[0] = 7;

    std::cout << "Used: " << a.used() << "\n";
  }

  std::cout << "After scope: " << a.used() << "\n";
}
```

All allocations inside the scope are automatically discarded.

## Features

-   Header-only
-   C++17 compatible
-   O(1) allocation
-   O(1) reset
-   Mark and rewind support
-   RAII scope helper
-   Alignment-aware allocation
-   Move-enabled arena
-   No global state

## API Overview

``` cpp
arena::Arena arena(capacity_bytes);

arena.allocate(size, alignment);
arena.try_allocate(size, alignment);

arena.make<T>(args...);
arena.make_array<T>(count);

arena.reset();

auto m = arena.mark();
arena.rewind(m);

arena::Arena::Scope scope(arena);
```

## Performance Model

This allocator:

-   Does not free individual allocations
-   Does not track allocation metadata
-   Does not shrink
-   Does not call destructors automatically

It is ideal when:

-   All allocations share the same lifetime
-   Memory is freed in bulk
-   Fragmentation must be avoided
-   Allocation speed is critical

## Thread Safety

`arena::Arena` is not thread-safe.

Use one arena per thread if needed.

## Tests

Run:

``` bash
vix build
vix tests
```

## License

MIT License
Copyright (c) Gaspard Kirira

