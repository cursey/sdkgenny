#include <iostream>
#include <sdkgenny.hpp>

int main(int argc, char* argv[]) {
    sdkgenny::Sdk sdk{};

    auto g = sdk.global_ns();
    g->type("int")->size(4);
    g->type("float")->size(4);

    auto foo = g->class_("Foo");
    foo->variable("a")->type("int")->append();
    foo->variable("b")->type("float")->append();

    auto bar = g->class_("Bar");
    bar->variable("c")->type("int")->append();

    auto baz = g->generic_type("Baz<Foo, Bar*>")->template_type(foo)->template_type(bar->ptr())->size(42);

    auto qux = g->class_("Qux");
    qux->variable("baz")->type(baz)->append();

    auto quux = g->class_("Quux");
    quux->function("hello")->param("name")->type(baz->ref());

    sdk.generate(std::filesystem::current_path() / "generic_type_sdk");

    return 0;
}