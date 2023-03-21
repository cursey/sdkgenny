// This example originally showcased an error in nested struct generation.
#include <iostream>

#include <sdkgenny_parser.hpp>

int main(int argc, char* argv[]) {
    sdkgenny::Sdk sdk{};
    sdkgenny::parser::State s{};
    s.parents.push_back(sdk.global_ns());

    tao::pegtl::string_input in{/*R"(
                                    namespace foo {
                                        struct bar {
                                            struct baz {}
                                        }
                                    }
                                )"*/
        R"(
            struct foo {
                struct bar* bar
            }

        )",
        ""};

    try {
        tao::pegtl::parse<sdkgenny::parser::Grammar, sdkgenny::parser::Action>(in, s);
    } catch (const tao::pegtl::parse_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    auto sdk_path = std::filesystem::current_path() / "nested_struct_sdk";
    std::filesystem::remove_all(sdk_path);
    sdk.generate(sdk_path);

    return 0;
}
