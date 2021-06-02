#include <iostream>
#include <cstdlib>
#include <optional>

#include <GennyParser.hpp>

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
type int 4
type float 4

# Make an actual namespace.
namespace foobar

# Make a class in the namespace.
struct Foo
    # Add some members.
    int a @ 0
    float b

# Make a subclass.
struct Bar : Foo
    # Add a member after 'b'.
    int c

# Make a subclass with multiple parents.
struct Baz : Foo, Bar 
    float d
)";

namespace pegtl = tao::pegtl;

struct State {
    genny::Namespace* global_ns{};
    genny::Namespace* cur_ns{};
    genny::Struct* cur_struct{};

    std::string type_name{};
    size_t type_size{};

    std::string struct_name{};
    std::vector<std::string> struct_parents{};

    std::string var_type{};
    std::string var_name{};
    std::optional<uintptr_t> var_offset{};

    std::vector<std::string> ns_pieces{};
};

template <typename Rule> struct Action : pegtl::nothing<Rule> {};

template <> struct Action<genny::parser::NsName> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.ns_pieces.emplace_back(in.string_view());
        std::cout << "Ns name: " << in.string_view() << std::endl;
    }
};

template <> struct Action<genny::parser::NsDecl> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        auto cur_ns = s.global_ns;

        for (auto&& ns : s.ns_pieces) {
            cur_ns = cur_ns->namespace_(ns);
        }

        s.cur_ns = cur_ns;
        s.ns_pieces.clear();
        std::cout << "Ns decl: " << in.string_view() << std::endl;
    }
};

template <> struct Action<genny::parser::TypeSize> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.type_size = std::stoull(in.string_view().data(), nullptr, 0);
        std::cout << "Type size: " << s.type_size << std::endl;
    }
};

template <> struct Action<genny::parser::TypeName> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.type_name = in.string_view();
        std::cout << "Type name: " << s.type_name << std::endl;
    }
};

template <> struct Action<genny::parser::TypeDecl> {
    template <typename ActionInput> static void apply(const ActionInput& in,State& s) {
        s.cur_ns->type(s.type_name)->size(s.type_size);
        s.type_name.clear();
        s.type_size = -1;
        std::cout << "Type decl: " << in.string_view() << std::endl;
    }
};

template <> struct Action<genny::parser::StructName> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.struct_name = in.string_view();
        std::cout << "Struct name: " << s.struct_name << std::endl;
    }
};

template <> struct Action<genny::parser::StructParent> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.struct_parents.emplace_back(in.string_view());
        std::cout << "Struct parent: " << in.string_view() << std::endl;
    }
};

template <> struct Action<genny::parser::StructDecl> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.cur_struct = s.cur_ns->struct_(s.struct_name);

        for (auto&& parent_name : s.struct_parents) {
            auto parent = s.cur_ns->find<genny::Struct>(parent_name);

            if (parent == nullptr) {
                throw tao::pegtl::parse_error{"Parent '" + parent_name + "' does not exist", in};
            }

            s.cur_struct->parent(parent);
        }
        
        s.struct_name.clear();
        s.struct_parents.clear();
        std::cout << "Struct decl: " << in.string_view() << std::endl;
    }
};

template <> struct Action<genny::parser::VarType> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) { 
        s.var_type = in.string_view();
        std::cout << "Var type: " << s.var_type << std::endl;
    }
};

template <> struct Action<genny::parser::VarName> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) { 
        s.var_name = in.string_view();
        std::cout << "Var name: " << s.var_name << std::endl;
    }
};

template <> struct Action<genny::parser::VarOffset> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) { 
        s.var_offset = std::stoull(in.string_view().data(), nullptr, 0);
        std::cout << "Var offset: " << *s.var_offset << std::endl;
    }
};

template <> struct Action<genny::parser::VarDecl> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) { 
        auto var = s.cur_struct->variable(s.var_name)->type(s.var_type);

        if (s.var_offset) {
            var->offset(*s.var_offset);
        } else {
            var->append();
        }

        s.var_type.clear();
        s.var_name.clear();
        s.var_offset = std::nullopt;

        std::cout << "Var decl: " << in.string_view() << std::endl;
    }
};

int main(int argc, char* argv[]) {
    genny::Sdk sdk{};
    State s{};
    s.global_ns = s.cur_ns = sdk.global_ns();

    //pegtl::string_input in{"float type 4", "example_string"};
    //pegtl::string_input in{g_example_str, "example_string"};
    pegtl::string_input in{g_usage_str, "usage_str"};

    try {
        auto r = pegtl::parse<genny::parser::Grammar, Action>(in, s);
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