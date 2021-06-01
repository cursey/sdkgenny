#include <Genny.hpp>

int main(int argc, char* argv[]) {
    // Make an SDK generator.
    genny::Sdk sdk{};

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