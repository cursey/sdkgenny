#include <sdkgenny.hpp>

int main(int argc, char* argv[]) {
    sdkgenny::Sdk sdk{};
    auto g = sdk.global_ns();

    auto i32 = g->type("int")->size(4);
    auto f32 = g->type("float")->size(4);
    auto char8 = g->type("char")->size(1);

    auto s = g->struct_("StructWithConstants");

    s->constant("SOME_INT")->integer(42)->type(i32);
    s->constant("SOME_OTHER_INT")->integer(777)->type(i32);
    s->constant("SOME_FLOAT")->real(123.456f)->type(f32);
    s->constant("SOME_STR")->string("Hello, world!")->type(char8->ptr());

    sdk.generate(std::filesystem::current_path() / "constants_sdk");

    return 0;
}
