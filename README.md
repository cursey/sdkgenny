# SdkGenny

SdkGenny is a library for generating C++ compatible SDKs for third party applications.

## Installation

This project uses CMake. Just get the `include/` and `src/` dirs integrated into your project in some way. The only dependency is on `PEGTL` which is only necessary if you're using the included parser.

## Usage

Here is a short example of how to use SdkGenny. This does not showcase every feature. For more examples check the `examples/` directory.

```C++
#include <sdkgenny.hpp>

int main(int argc, char* argv[]) {
    // Make an SDK generator.
    sdkgenny::Sdk sdk{};

    // Get the global namespace for the SDK.
    auto g = sdk.global_ns();

    // Add some basic types to the global namespace.
    g->type("int")->size(4);
    g->type("float")->size(4);

    // Make an actual namespace.
    auto ns = g->namespace_("foobar");

    // Make a class in the namespace.
    auto foo = ns->class_("Foo");

    // Add some members.
    foo->variable("a")->type("int")->offset(0);
    foo->variable("b")->type("float")->append();

    // Make a subclass.
    auto bar = ns->class_("Bar")->parent(foo);

    // Add a member after 'b'.
    bar->variable("c")->type("int")->append();

    // Generate the SDK to the "usage_sdk" folder.
    sdk.generate(std::filesystem::current_path() / "usage_sdk");

    return 0;
}
```

Will produce the following 2 files

### `foobar/Foo.hpp`

```C++
#pragma once
namespace foobar {
#pragma pack(push, 1)
class Foo {
public:
    int a; // 0x0
    float b; // 0x4
}; // Size: 0x8
#pragma pack(pop)
}
```

### `foobar/Bar.hpp`

```C++
#pragma once
#include ".\Foo.hpp"
namespace foobar {
#pragma pack(push, 1)
class Bar : public Foo {
public:
    int c; // 0x8
}; // Size: 0xc
#pragma pack(pop)
}
```

## Projects

* [REFramework](https://github.com/praydog/REFramework) by [@praydog](https://github.com/praydog) - A mod framework for Resident-Evil 2
* [GlacierGenny](https://github.com/praydog/GlacierGenny) by [@praydog](https://github.com/praydog) - An SDK generator for HITMAN3
* [UE4Genny](https://github.com/cursey/ue4genny) - An SDK generator for Unreal Engine 4 games
* [luagenny](https://github.com/praydog/luagenny) - Lua bindings & utilities for sdkgenny
* [REGenny](https://github.com/cursey/regenny) - A reverse engineering tool to interactively reconstruct structures

## License

[MIT](https://choosealicense.com/licenses/mit/)
