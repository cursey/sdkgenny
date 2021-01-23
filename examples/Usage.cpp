#include <iostream>

#include <Genny.hpp>

int main(int argc, char* argv[]) {
    // Make our SDK's header file.
    auto sdk = std::make_unique<genny::HeaderFile>("Usage.hpp");

    // Add some basic types to the global namespace.
    sdk->type("int")->size(4);
    sdk->type("float")->size(4);

    // Make an actual namespace.
    auto ns = sdk->namespace_("foobar");

    // Make a class in the namespace.
    auto foo = ns->class_("Foo");

    // Add some members.
    foo->variable("a")->type("int")->offset(0);
    foo->variable("b")->type("float")->offset(4);

    // Make a subclass.
    auto bar = ns->class_("Bar")->parent(foo);

    // Add a member after 'b'.
    bar->variable("c")->type("int")->offset(foo->variable("b")->end());

    // Generate the sdk.
    sdk->generate(std::cout);

    return 0;

}