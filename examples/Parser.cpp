#include <iostream>
#include <cstdlib>
#include <optional>

#include <Genny.hpp>

constexpr auto g_example_str = R"(
type float 4
type double 8

struct vec3
    float x @ 0 # omg thats @ 0
    float y # This will follow the x variable and land @ 4
    float z # This will follow y and land @ 8

# The total size will be... 12!
)";

constexpr auto g_usage_str = R"(
# Add some basic types to the global namespace.
type char 1 [[i8]]
type int 4 [[i32]]
type float 4 [[f32]]

# Make an actual namespace.
namespace foo.bar

# Make a class in the namespace.
struct Foo
    # Add some members.
    int a @ 0 [[u32]]
    float b

# Make a subclass.
struct Bar : Foo 
    # Add a member after 'b'.
    int c

# Make a subclass with multiple parents.
struct Baz : Foo, Bar
    float d

namespace baz

struct Qux
    foo.bar.Baz baz
    char* str
    char*[10] str_x_10

namespace

struct Vec3
    float x
    float y
    float z

struct OtherVec3
    float[3] xyz
    float* xyz_ptr
    int** xyz_ptr_ptr

struct Mat4x3
    float[4][3] m

enum Color
    RED = 1
    BLUE = 2
    GREEN = 0x3

enum Size : char
    SMALL = 0
    MEDIUM = 1
    LARGE = 2
    XLARGE = 3
    XXLARGE = 4

enum class Speed : int 
    SLOW = 25 
    MEDIUM = 40
    FAST = 65
)";

namespace pegtl = tao::pegtl;

int main(int argc, char* argv[]) {
    genny::Sdk sdk{};
    genny::parser::State s{};
    s.global_ns = s.cur_ns = sdk.global_ns();

    //pegtl::string_input in{"float type 4", "example_string"};
    //pegtl::string_input in{g_example_str, "example_string"};
    pegtl::string_input in{g_usage_str, "usage_str"};

    try {
        auto r = pegtl::parse<genny::parser::Grammar, genny::parser::Action>(in, s);
        std::cout << r << std::endl;
    } catch (const pegtl::parse_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    auto sdk_path = std::filesystem::current_path() / "parser_sdk";
    std::filesystem::remove_all(sdk_path);
    sdk.generate(sdk_path);

    return 0;
}