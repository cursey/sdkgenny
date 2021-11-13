#include <Genny.hpp>

int main(int argc, char* argv[]) {
    genny::Sdk sdk{};
    
    auto g = sdk.global_ns();
    auto bad_type = g->type("Bad name type")->size(8);
    auto bad_class = g->class_("Bad class");

    bad_class->variable("123 bad variable")->type(bad_type)->offset(0);
    bad_class->variable("shorthand bad variable")->type("Bad name type")->offset(8);

    sdk.generate(std::filesystem::current_path() / "bad_name_sdk");

    return 0;
}