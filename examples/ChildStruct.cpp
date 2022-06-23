// This example originally showcased an error in dependency resolving for structs declared within other structs.
#include "Genny.hpp"

int main(int argc, char* argv[]) {
    genny::Sdk sdk{};
    auto g = sdk.global_ns();

    auto i32 = g->type("int")->size(4);
    auto foo = g->struct_("Foo");

    foo->variable("a")->type(i32)->append();

    auto bar = foo->struct_("Bar");

    bar->variable("b")->type(i32)->append();

    auto baz = foo->struct_("Baz");

    baz->variable("c")->type(i32)->append();

    auto qux = g->struct_("Qux");

    // qux->variable("d")->type(bar)->append();
    qux->variable("e")->type(baz->ptr())->append();

    sdk.generate(std::filesystem::current_path() / "child_struct_sdk");

    return 0;
}
