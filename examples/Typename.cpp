#include <iostream>

#include "Genny.hpp"

int main(int argc, char* argv[]) {
    auto g = std::make_unique<genny::HeaderFile>("Typename.hpp");

    auto foo = g->namespace_("foo")->namespace_("baz")->class_("Foo");
    foo->variable("bar")->type(g->namespace_("bar")->class_("Bar")->ptr());

    auto bar = g->namespace_("bar")->class_("Bar");
    bar->variable("foo")->type(foo->ptr());

    g->generate(std::cout);

    return 0;
}