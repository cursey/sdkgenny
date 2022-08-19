// This example originally showcased an error in nested struct generation.
#include <iostream>

#include <GennyParser.hpp>

int main(int argc, char* argv[]) {
    genny::Sdk sdk{};
    genny::parser::State s{};
    s.parents.push_back(sdk.global_ns());

    tao::pegtl::string_input in{
        R"(
            namespace foo {
                struct bar {
                    struct baz {}
                }
            }
        )"
        , ""};

    try {
        tao::pegtl::parse<genny::parser::Grammar, genny::parser::Action>(in, s);
    } catch (const tao::pegtl::parse_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    auto sdk_path = std::filesystem::current_path() / "nested_struct_sdk";
    std::filesystem::remove_all(sdk_path);
    sdk.generate(sdk_path);

    return 0;
}
