# SdkGenny

SdkGenny is a single file header framework for generating C++ compatible SDKs for third party applications.

## Installation

Copy `Genny.hpp` from the `include/` directory into your project and `#include` it when you want to use it.

## Usage
Here is a short example of how to use SdkGenny. This does not showcase every feature. For more examples check the `examples/` directory.
```
// Make our SDK's header file.
auto sdk = std::make_unique<genny::HeaderFile>("Sdk.hpp");

// Add some basic types to the global namespace.
sdk->type("int")->size(4);
sdk->type("float")->size(4);

// Make an actual namespace.
auto ns = sdk->namespace_("foobar");

// Make a class in the namespace.
auto foo = ns->class_("Foo");

// Add some members.
foo->member("a")->type("int")->offset(0);
foo->member("b")->type("float")->offset(4);

// Make a subclass.
auto bar = ns->class_("Bar")->parent(foo);

// Add a member after 'b'.
bar->member("c")->type("int")->offset(foo->member("b")->end());

// Generate the sdk.
sdk->generate(std::cout);
```
Will produce the following output:
```
// Sdk.hpp

#pragma once

#pragma pack(push, 1)

namespace foobar {
class Foo;
class Bar;

class Foo {
public:
    int a; // 0x0
    float b; // 0x4
}; // Size: 0x8

class Bar : public Foo {
public:
    int c; // 0x8
}; // Size: 0xc

} // namespace foobar

#pragma pack(pop)
```

## License
[MIT](https://choosealicense.com/licenses/mit/)