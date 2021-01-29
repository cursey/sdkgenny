#include <Genny.hpp>

int main(int argc, char* argv[]) {
    genny::Sdk sdk{};
    auto g = sdk.global_ns();

    auto foo = g->namespace_("foo")->namespace_("baz")->class_("Foo");
    foo->variable("bar")->type(g->namespace_("bar")->class_("Bar")->ptr());

    auto bar = g->namespace_("bar")->class_("Bar");
    bar->variable("foo")->type(foo->ptr());

    for (int i = 0; i < 100; ++i) {
        g->namespace_("bs_ns_" + std::to_string(i));
    }

    sdk.generate(std::filesystem::current_path() / "typename_sdk");

    return 0;
}