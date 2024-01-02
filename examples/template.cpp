#include <print>

#include <sdkgenny.hpp>

int main(int argc, char* argv[]) {
    sdkgenny::Sdk sdk{};
    auto* g = sdk.global_ns();

    auto* i32 = g->type("int")->size(4);
    auto* foo = g->template_("Foo");

    foo->variable("a")->template_param("T")->offset(0);

    auto* bar = foo->instance({{"T", i32}});

    std::println("{} is {} bytes", bar->name(), bar->size());

    return 0;
}