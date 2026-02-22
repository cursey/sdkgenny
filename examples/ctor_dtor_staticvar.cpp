#include <sdkgenny.hpp>
#include <filesystem>

using namespace sdkgenny;

int main()
{
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
    auto n_var = foo->variable("b")->type("float")->append();
    n_var->initializer("10");

    // Make a subclass.
    auto bar = ns->class_("Bar")->parent(foo);

    auto dtor = bar->destructor();
    auto ctor = bar->constructor();
    ctor->initializer_list("a(1)"); // example
    ctor->param("test")->type(g->type("int"));
    ctor->procedure("int a = 2");

    // Use the 'as' template to perform upcasting more efficiently
    auto var = bar->static_variable("c")->type(g->type("int")->size(4)->array_(2))->append()->as<StaticVariable>();
    
    // not necessary:
    //auto static_var = (StaticVariable*)var; // upcasting

    var->initializer("100");
    sdk.generate(std::filesystem::current_path() / "usage_sdk");
    return 0;
}
