#include <cstdlib>
#include <iostream>

#include <sdkgenny_parser.hpp>

// Reproduces the parser bug where enum values greater than UINT32_MAX trigger
// "stoul argument out of range" (or get silently truncated on platforms where
// std::stoul is 64-bit). The enum value field must be wide enough to hold the
// values the underlying type can express.
constexpr auto g_big_enum = R"(
type uint64_t 8

enum class Flags : uint64_t {
    NONE = 0,
    SMALL = 0x1,
    BIG = 0xDEADBEEFCAFEBABE,
    MAX = 0xFFFFFFFFFFFFFFFF,
}
)";

namespace pegtl = tao::pegtl;

int main() {
    sdkgenny::Sdk sdk{};
    sdkgenny::parser::State s{};
    s.parents.push_back(sdk.global_ns());

    pegtl::string_input in{g_big_enum, "big_enum"};

    try {
        pegtl::parse<sdkgenny::parser::Grammar, sdkgenny::parser::Action>(in, s);
    } catch (const std::exception& e) {
        std::cerr << "parse failed: " << e.what() << std::endl;
        return 1;
    }

    auto sdk_path = std::filesystem::current_path() / "bigenum_sdk";
    std::filesystem::remove_all(sdk_path);
    sdk.generate(sdk_path);

    std::cout << sdk_path.string() << std::endl;
    return 0;
}
