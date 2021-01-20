#include <iostream>

#include <Genny.hpp>

int main(int argc, char* argv[]) {
    // Make the global namespace (empty name).
    auto g = std::make_unique<genny::Namespace>("");

    // Add some basic types to the global namespace.
    g->type("int")->size(4);
    g->type("float")->size(4);

    // Make an actual namespace.
    auto ns = g->namespace_("foobar");

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
    g->generate(std::cout);

    return 0;

}