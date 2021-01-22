#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace genny {

class Type;
class Pointer;
class Struct;
class Class;
class Enum;
class Variable;
class Function;

class Indent : public std::streambuf {
public:
    explicit Indent(std::streambuf* dest, int indent = 4) : m_dest{dest}, m_indent(indent, ' ') {}
    explicit Indent(std::ostream& dest, int indent = 4) : m_dest{dest.rdbuf()}, m_indent(indent, ' '), m_owner{&dest} {
        m_owner->rdbuf(this);
    }
    virtual ~Indent() {
        if (m_owner != nullptr) {
            m_owner->rdbuf(m_dest);
        }
    }

protected:
    int overflow(int ch) override {
        if (m_is_at_start_of_line && ch != '\n') {
            m_dest->sputn(m_indent.data(), m_indent.size());
        }
        m_is_at_start_of_line = ch == '\n';
        return m_dest->sputc(ch);
    }

private:
    std::streambuf* m_dest{};
    bool m_is_at_start_of_line{true};
    std::string m_indent{};
    std::ostream* m_owner{};
};

class Object {
public:
    static Object* static_class() {
        static Object object{};
        return &object;
    }

    Object(std::string_view name) : m_name{name}, m_object_class{this} {}
    virtual ~Object() = default;

    const auto& name() const { return m_name; }

    virtual void generate(std::ostream& os) const {};

    template <typename T> bool is_a() const {
        for (auto i = m_object_class; i != nullptr; i = i->m_parent_class) {
            if (i == T::static_class()) {
                return true;
            }
        }

        return false;
    }

    // Searches for an owner of the correct type.
    template <typename T> T* owner() {
        for (auto owner = m_owner; owner != nullptr; owner = owner->m_owner) {
            if (owner->is_a<T>()) {
                return (T*)owner;
            }
        }

        return nullptr;
    }

protected:
    friend class Type;
    friend class Namespace;

    Object* m_object_class{};
    Object* m_parent_class{};
    Object* m_owner{};

    std::string m_name{};
    std::vector<std::unique_ptr<Object>> m_children{};

    template <typename T> T* add(std::unique_ptr<T> object) {
        object->m_owner = this;
        return (T*)m_children.emplace_back(std::move(object)).get();
    }

    template <typename T> T* find(std::string_view name) const {
        for (auto&& child : m_children) {
            if (child->is_a<T>() && child->m_name == name) {
                return (T*)child.get();
            }
        }

        return nullptr;
    }

    template <typename T> T* find_in_owners(std::string_view name, bool include_self) const {
        auto owner = (include_self) ? this : m_owner;

        for (; owner != nullptr; owner = owner->m_owner) {
            if (auto search = owner->find<T>(name)) {
                return search;
            }
        }

        return nullptr;
    }

    template <typename T> T* find_or_add(std::string_view name) {
        if (auto search = find<T>(name)) {
            return search;
        }

        return add(std::make_unique<T>(name));
    }

    template <typename T> T* find_in_owners_or_add(std::string_view name) {
        if (auto search = find_in_owners<T>(name, true)) {
            return search;
        }

        return add(std::make_unique<T>(name));
    }

    template <typename T> std::vector<T*> get_all() const {
        std::vector<T*> children{};

        for (auto&& child : m_children) {
            if (child->is_a<T>()) {
                children.emplace_back((T*)child.get());
            }
        }

        return children;
    }

    template <typename T> bool has_any() const {
        for (auto&& child : m_children) {
            if (child->is_a<T>()) {
                return true;
            }
        }

        return false;
    }


private:
    Object() : m_object_class{this} {}
};

template <typename T> T* cast(const Object* object) {
    if (object->is_a<T>()) {
        return (T*)object;
    }

    return nullptr;
}


#define SDK_OBJECT(T, TParent)                    \
public:                                           \
    static T* static_class() {                    \
        static T static_t{};                      \
        return &static_t;                         \
    }                                             \
                                                  \
private:                                          \
    T() : TParent{""} {                           \
        m_object_class = this;                    \
        m_parent_class = TParent::static_class(); \
    }                                             \
                                                  \
public:                                           \
    T(std::string_view name) : TParent{name} {    \
        m_object_class = static_class();          \
        m_parent_class = TParent::static_class(); \
    }

class Type : public Object {
public:
    SDK_OBJECT(Type, Object);

    virtual size_t size() const { return m_size; }
    auto size(int size) {
        m_size = size;
        return this;
    }

    virtual void generate_typename(std::ostream& os) const { os << m_name; }

    Pointer* ptr();

protected:
    size_t m_size{};
};

class Pointer : public Type {
public:
    SDK_OBJECT(Pointer, Type);

    auto to() const { return m_to; }
    auto to(Type* to) {
        m_to = to;
        return this;
    }

    size_t size() const override { return sizeof(uintptr_t); }
    void generate_typename(std::ostream& os) const override {
        m_to->generate_typename(os);
        os << "*";
    }

protected:
    Type* m_to;
};

inline Pointer* Type::ptr() {
    return m_owner->find_or_add<Pointer>(m_name)->to(this);
}

class Variable : public Object {
public:
    SDK_OBJECT(Variable, Object);

    auto type() const { return m_type; }
    auto type(Type* type) {
        m_type = type;
        return this;
    }

    // Helper that recurses though owners to find the correct type.
    auto type(std::string_view name) {
        m_type = find_in_owners_or_add<Type>(name);
        return this;
    }

    auto offset() const { return m_offset; }
    auto offset(uintptr_t offset) {
        m_offset = offset;
        return this;
    }

    virtual size_t size() const {
        if (m_type == nullptr) {
            return 0;
        }

        return m_type->size();
    }

    auto end() const { return offset() + size(); }

    void generate(std::ostream& os) const override {
        m_type->generate_typename(os);
        os << " " << m_name << "; // 0x" << std::hex << m_offset << "\n";
    }

protected:
    Type* m_type{};
    uintptr_t m_offset{};
};

class Array : public Variable {
public:
    SDK_OBJECT(Array, Variable);

    auto count() const { return m_count; }
    auto count(size_t size) {
        m_count = size;
        return this;
    }

    size_t size() const override { return m_type->size() * m_count; }

    void generate(std::ostream& os) const override {
        m_type->generate_typename(os);
        os << " " << m_name << "[" << std::dec << m_count << "]; // 0x" << std::hex << m_offset << "\n";
    }

protected:
    size_t m_count{};
};

class Parameter : public Object {
public:
    SDK_OBJECT(Parameter, Object);

    auto type() const { return m_type; }
    auto type(Type* type) {
        m_type = type;
        return this;
    }

    void generate(std::ostream& os) const override {
        m_type->generate_typename(os);
        os << " " << m_name;
    }

protected:
    Type* m_type{};
};

class Function : public Object {
public:
    SDK_OBJECT(Function, Object);

    auto param(std::string_view name) { return find_or_add<Parameter>(name); }

    auto returns() const { return m_return_value; }
    auto returns(Type* return_value) {
        m_return_value = return_value;
        return this;
    }

    auto procedure() const { return m_procedure; }
    auto procedure(std::string_view procedure) {
        m_procedure = procedure;
        return this;
    }

    void generate(std::ostream& os) const override {
        generate_prototype(os);
        generate_procedure(os);
    }

protected:
    Type* m_return_value{};
    std::string m_procedure{};

    void generate_prototype(std::ostream& os) const {
        if (m_return_value == nullptr) {
            os << "void";
        } else {
            m_return_value->generate_typename(os);
        }

        os << " " << m_name << "(";

        auto is_first_param = true;

        for (auto&& param : get_all<Parameter>()) {
            if (is_first_param) {
                is_first_param = false;
            } else {
                os << ", ";
            }

            param->generate(os);
        }

        os << ")";
    }

    void generate_procedure(std::ostream& os) const {
        if (m_procedure.empty()) {
            os << " {}\n";
        } else {
            os << " {\n";
            {
                Indent _{os};
                os << m_procedure;
            }
            os << "\n}\n";
        }
    }
};

class VirtualFunction : public Function {
public:
    SDK_OBJECT(VirtualFunction, Function);

    void generate(std::ostream& os) const override {
        os << "virtual ";
        generate_prototype(os);

        if (m_procedure.empty()) {
            os << " = 0;\n";
        } else {
            generate_procedure(os);
        }
    }
};

class Enum : public Type {
public:
    SDK_OBJECT(Enum, Type);

    auto value(std::string_view name, uint64_t value) {
        for (auto&& [val_name, val_val] : m_values) {
            if (val_name == name) {
                val_val = value;
                return this;
            }
        }

        m_values.emplace_back(name, value);
        return this;
    }

    auto type() const { return m_type; }
    auto type(Type* type) {
        m_type = type;
        return this;
    }

    size_t size() const override {
        if (m_type == nullptr) {
            return sizeof(int);
        } else {
            return m_type->size();
        }
    }

    void generate(std::ostream& os) const override {
        os << "enum " << m_name;
        generate_type(os);
        os << " {\n";
        generate_enums(os);
        os << "};\n";
    }

protected:
    std::vector<std::tuple<std::string, uint64_t>> m_values{};
    Type* m_type{};

    void generate_type(std::ostream& os) const {
        if (m_type != nullptr) {
            os << " : ";
            m_type->generate_typename(os);
        }
    }

    void generate_enums(std::ostream& os) const {
        Indent _{os};

        for (auto&& [name, value] : m_values) {
            os << name << " = " << value << ",\n";
        }
    }
};

class EnumClass : public Enum {
public:
    SDK_OBJECT(EnumClass, Enum);

    void generate(std::ostream& os) const override {
        os << "enum class " << m_name;
        generate_type(os);
        os << " {\n";
        generate_enums(os);
        os << "};\n";
    }
};

class Struct : public Type {
public:
    SDK_OBJECT(Struct, Type);

    auto member(std::string_view name) { return find_or_add_unique<Variable>(name); }
    auto array_(std::string_view name) { return find_or_add_unique<Array>(name); }
    auto struct_(std::string_view name) { return find_or_add_unique<Struct>(name); }
    auto class_(std::string_view name) { return find_or_add_unique<Class>(name); }
    auto enum_(std::string_view name) { return find_or_add_unique<Enum>(name); }
    auto enum_class(std::string_view name) { return find_or_add_unique<EnumClass>(name); }
    auto method(std::string_view name) { return find_or_add_unique<Function>(name); }
    auto virtual_method(std::string_view name) { return find_or_add_unique<VirtualFunction>(name); }

    auto parent() const { return m_parent; }
    auto parent(Struct* parent) {
        m_parent = parent;
        return this;
    }

    size_t size() const override {
        auto size = m_size;

        if (m_parent != nullptr) {
            size = m_parent->size();
        }

        if (has_any<VirtualFunction>()) {
            size += sizeof(uintptr_t);
        }

        for (auto&& var : get_all<Variable>()) {
            auto var_end = var->end();

            if (var_end > size) {
                size = var_end;
            }
        }

        return size;
    }
    auto size(int size) {
        m_size = size;
        return this;
    }

    virtual void generate_forward_decl(std::ostream& os) const { os << "struct " << m_name << ";\n"; }

    void generate(std::ostream& os) const override {
        os << "struct " << m_name;
        generate_inheritance(os);
        os << " {\n";
        generate_internal(os);
        os << "}; // Size: 0x" << std::hex << size() << "\n";
    }

protected:
    Struct* m_parent{};

    template <typename T> T* find_or_add_unique(std::string_view name) {
        if (auto search = find<T>(name); search != nullptr) {
            return search;
        }

        std::string fixed_name{};
        auto num_collisions = 0;
        auto has_collision = false;

        do {
            has_collision = false;

            for (auto parent = m_parent; parent != nullptr; parent = parent->m_parent) {
                if (m_parent->find<Object>(fixed_name.empty() ? name : fixed_name) != nullptr) {
                    fixed_name = name;
                    fixed_name += std::to_string(num_collisions);
                    ++num_collisions;
                    has_collision = true;
                }
            }
        } while (has_collision);

        if (num_collisions == 0) {
            return add(std::make_unique<T>(name));
        }

        return add(std::make_unique<T>(fixed_name));
    }

    void generate_inheritance(std::ostream& os) const {
        if (m_parent == nullptr) {
            return;
        }

        os << " : public " << m_parent->m_name;
    }

    void generate_internal(std::ostream& os) const {
        Indent _{os};

        for (auto&& child : get_all<Enum>()) {
            child->generate(os);
            os << "\n";
        }

        for (auto&& child : get_all<Struct>()) {
            child->generate(os);
            os << "\n";
        }

        std::unordered_map<std::uintptr_t, Variable*> var_map{};

        for (auto&& var : get_all<Variable>()) {
            var_map[var->offset()] = var;
        }

        auto max_offset = size();
        size_t offset = 0;

        // Skip over the vtable.
        if (has_any<VirtualFunction>()) {
            offset = sizeof(uintptr_t);
        }

        // Start off where the parent ends.
        if (m_parent != nullptr) {
            offset = m_parent->size();
        }

        auto last_offset = offset;

        while (offset < max_offset) {
            if (auto search = var_map.find(offset); search != var_map.end()) {
                auto var = search->second;

                // Skip variables where the user has not given us a valid size (forgot to set a type or the type is unfinished).
                if (var->size() == 0) {
                    ++offset;
                    continue;
                }

                if (offset - last_offset > 0) {
                    os << "char pad_" << std::hex << last_offset << "[0x" << std::hex << offset - last_offset << "];\n";
                }

                var->generate(os);

                offset += var->size();
                last_offset = offset;
            } else {
                ++offset;
            }
        }

        if (offset - last_offset > 0) {
            os << "char pad_" << std::hex << last_offset << "[0x" << std::hex << offset - last_offset << "];\n";
        }

        if (has_any<Function>()) {
            os << "\n";

            // Generate a default destructor to force addition of the vtable ptr.
            if (has_any<VirtualFunction>()) {
                os << "virtual ~" << m_name << "() = default;\n";
            }

            for (auto&& child : get_all<Function>()) {
                child->generate(os);
            }
        }
    }
};

class Class : public Struct {
public:
    SDK_OBJECT(Class, Struct);

    void generate_forward_decl(std::ostream& os) const { os << "class " << m_name << ";\n"; }

    void generate(std::ostream& os) const override {
        os << "class " << m_name;
        generate_inheritance(os);
        os << " {\n";
        os << "public:\n";
        generate_internal(os);
        os << "}; // Size: 0x" << std::hex << size() << "\n";
    }
};

class Namespace : public Object {
public:
    SDK_OBJECT(Namespace, Object);

    auto type(std::string_view name) { return find_in_owners_or_add<Type>(name); }
    auto struct_(std::string_view name) { return find_or_add<Struct>(name); }
    auto class_(std::string_view name) { return find_or_add<Class>(name); }
    auto enum_(std::string_view name) { return find_or_add<Enum>(name); }
    auto enum_class(std::string_view name) { return find_or_add<EnumClass>(name); }
    auto namespace_(std::string_view name) { return find_or_add<Namespace>(name); }

    void generate(std::ostream& os) const override {
        if (!m_name.empty()) {
            os << "namespace " << m_name << " {\n";
        }

        generate_internal(os);

        if (!m_name.empty()) {
            os << "} // namespace " << m_name << "\n";
        }
    }

protected:
    void generate_internal(std::ostream& os) const {
        for (auto&& child : get_all<Namespace>()) {
            child->generate(os);
            os << "\n";
        }

        for (auto&& child : get_all<Enum>()) {
            child->generate(os);
            os << "\n";
        }

        if (has_any<Struct>()) {
            for (auto&& child : get_all<Struct>()) {
                child->generate_forward_decl(os);
            }

            os << "\n";

            std::unordered_set<const Struct*> generated_structs{};
            std::function<void(const Struct*)> generate_struct = [&](const Struct* struct_) {
                if (generated_structs.find(struct_) != generated_structs.end()) {
                    return;
                }

                if (auto parent = struct_->parent()) {
                    generate_struct(parent);
                }

                struct_->generate(os);
                os << "\n";
                generated_structs.emplace(struct_);
            };

            for (auto&& child : get_all<Struct>()) {
                generate_struct(child);
            }
        }
    }
};

class HeaderFile : public Namespace {
public:
    SDK_OBJECT(HeaderFile, Namespace);

    auto preamble(std::string_view preamble) { m_preamble = preamble;
        return this;
    }
    auto postamble(std::string_view postamble) {
        m_postamble = postamble;
        return this;
    }

    auto include(std::string_view header) {
        m_includes.emplace(header);
        return this;
    }
    auto include_local(std::string_view header) {
        m_local_includes.emplace(header);
        return this;
    }
    
    void generate(std::ostream& os) const override { 
        if (!m_name.empty()) {
            os << "// " << m_name << "\n";
        }

        if (!m_preamble.empty()) {
            std::istringstream sstream{m_preamble};
            std::string line{};

            os << "\n";

            while (std::getline(sstream, line)) {
                os << "// " << line << "\n";
            }
        }

        os << "\n";
        os << "#pragma once\n";

        if (!m_includes.empty()) {
            os << "\n";

            for (auto&& include : m_includes) {
                os << "#include <" << include << ">\n";
            }
        }

        if (!m_local_includes.empty()) {
            os << "\n";

            for (auto&& include : m_local_includes) {
                os << "#include \"" << include << "\"\n";
            }
        }

        auto has_structs = has_any<Struct>();

        if (has_structs) {
            os << "\n#pragma pack(push, 1)\n\n";
        } else {
            os << "\n";
        }

        generate_internal(os);

        if (has_structs) {
            os << "#pragma pack(pop)\n";
        }

        if (!m_postamble.empty()) {
            os << "\n";

            std::istringstream sstream{m_postamble};
            std::string line{};

            while (std::getline(sstream, line)) {
                os << "// " << line << "\n";
            }
        }

        os << "\n";
    }

private:
    std::string m_preamble{};
    std::string m_postamble{};
    std::set<std::string> m_includes{};
    std::set<std::string> m_local_includes{};
};

} // namespace genny