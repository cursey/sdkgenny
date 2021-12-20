#include <cstdlib>
#include <iostream>
#include <optional>

#include <GennyParser.hpp>

constexpr auto g_example_str = R"(
type float 4
type double 8

struct vec3
    float x @ 0 // omg thats @ 0
    float y // This will follow the x variable and land @ 4
    float z // This will follow y and land @ 8

// The total size will be... 12!
)";

constexpr auto g_usage_str = R"(
// Add some basic types to the global namespace.
type char 1 [[i8 ]];
type int 4 [[ i32]]
type float 4 [[f32]]

// Make an actual namespace.
namespace foo.bar {

// Make a class in the namespace.
struct Foo 0x10 {
    // Add some members.
    int a @ 0 [[u32]]
    float b
};

// Make a subclass.
struct Bar : Foo 0x20 {
    // Add a member after 'b'.
    int c
}

// Make a subclass with multiple parents.
struct Baz : Foo, Bar {
    float d 
}
}

namespace baz {


/* this is a cool struct
 * and this is a long comment
 * how neat. */
struct Qux  { 
    foo.bar.Baz baz
    char* str
    char* str_array[10];
    int add(int a, /* bad place for a comment but w/e */ int b)   
    int sub(int a, int b, int* c)
}

}

struct Vec3 {
    float x;
    float y;
    float z;
    float length    ( );
    Vec3 add( Vec3 other );
    static Vec3 zero();
};

struct OtherVec3 {
    float xyz[3]
    float* xyz_ptr
    int** xyz_ptr_ptr // woah
}

struct Mat4x3 {
    float m[4][3]
    float[4] n[3]; // Very strange.
    float[4][3] o
}

enum Color {
    RED = 1,
    BLUE = 2,
    GREEN = 0x3
}

namespace enums {
    
enum Size : char {
    SMALL = 0,
    MEDIUM = 1,
    LARGE = 2,
    XLARGE = 3,
    XXLARGE = 4,
}

enum class Speed : int {
    SLOW = 25,
    MEDIUM = 40,
    FAST = 65
};
}

type uint16_t 2 [[u16]]

struct Date {
    uint16_t nWeekDay : 3
    uint16_t nMonthDay : 6
    uint16_t nMonth : 5
    uint16_t nYear : 8
    Vec3 v
}

struct Nested {
    enum Enum {
        HELLO = 0,
        WORLD = 1,
    }

    struct Struct {
        int a
        int b +4
        int c +8
    }

    Enum enum_
    Struct struct_
}

class CoolClass {
    Nested nest
    virtual int say_hi(char* name)
    virtual void say_hello_world() @ 1
    virtual void say_goodbye() @ 10
}
)";

constexpr auto g_ns_bug = R"(
type int 4

namespace qux
struct foo
    int a

namespace bar
    struct baz : qux.foo
        int b

namespace   
    struct quux
        int c
)";

constexpr auto g_new = R"(
type int 4
type char 1

namespace enums {
enum Size : char {
    SMALL = 0,
    MEDIUM = 1,
    LARGE = 2,
    XLARGE = 3,
    XXLARGE = 4
}

namespace this_enum.is.nested.deep {

    enum class Speed : int {
        SLOW = 25,
        MEDIUM = 40,    
        FAST = 65
    }

}
}

namespace structs {
struct Vec3i {
    int x
    int y
    int z
}
}
)";

constexpr auto g_include = R"(
#include "types.genny"

struct Vec3f {
    float x
    float y
    float z
}

struct Vec3i {
    int x
    int y
    int z
}

struct Baz : foobar.Bar {
    int c
}
)";

constexpr auto g_fwd_decl_members = R"(
class Foo {
    struct Bar* bar
}

struct Bar {
    class Foo* foo
    enum class Baz* baz
}

enum class Baz {
    A = 0, 
    B = 1, 
    C = 2
}
)";

constexpr auto g_reclass = R"(
type char 1

// Created with ReClass.NET 1.2 by KN4CK3R

class Foo
{
public:
	char pad_0000[128]; //0x0000
}; //Size: 0x0080
static_assert(sizeof(Foo) == 0x80);

class Bar
{
public:
	char pad_0000[128]; //0x0000
}; //Size: 0x0080
static_assert(sizeof(Bar) == 0x80);

class Baz
{
public:
	class Foo *foo; //0x0000
	class Bar *bar; //0x0008
	char pad_0010[112]; //0x0010
}; //Size: 0x0080
static_assert(sizeof(Baz) == 0x80);

class Qux : public Baz
{
public:
	char pad_0080[120]; //0x0080
}; //Size: 0x00F8
static_assert(sizeof(Qux) == 0xF8);
)";

namespace pegtl = tao::pegtl;

int main(int argc, char* argv[]) {
    genny::Sdk sdk{};

    sdk.header_extension(".hxx");
    sdk.source_extension(".cxx");

    genny::parser::State s{};
    // s.global_ns = s.cur_ns = sdk.global_ns();
    s.parents.push_back(sdk.global_ns());

    // pegtl::string_input in{"float type 4", "example_string"};
    // pegtl::string_input in{g_example_str, "example_string"};
    pegtl::string_input in{g_usage_str, "usage_str"};
    // pegtl::string_input in{g_ns_bug, "ns_bug_str"};
    // pegtl::string_input in{g_new, "new_str"};
    // pegtl::string_input in{g_include, "include_str"};
    // pegtl::string_input in{g_fwd_decl_members, "fwd_decl_members"};
    // pegtl::string_input in{g_reclass, "reclass"};

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

    std::cout << sdk_path.string() << std::endl;

    return 0;
}