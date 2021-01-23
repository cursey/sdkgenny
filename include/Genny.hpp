#pragma once

#include <climits>
#include <functional>
#include <memory>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
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
    Object() = delete;
    Object(std::string_view name) : m_name{name} {}
    virtual ~Object() = default;

    const auto& name() const { return m_name; }

    template <typename T> bool is_a() const { return dynamic_cast<const T*>(this) != nullptr; }

    // Searches for an owner of the correct type.
    template <typename T> const T* owner() const {
        for (auto owner = m_owner; owner != nullptr; owner = owner->m_owner) {
            if (owner->is_a<T>()) {
                return (const T*)owner;
            }
        }

        return nullptr;
    }

    template <typename T> T* owner() { return (T*)((const Object*)this)->owner<T>(); }

protected:
    friend class Type;
    friend class Namespace;

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

    template <typename T, typename... TArgs> T* find_or_add(std::string_view name, TArgs... args) {
        if (auto search = find<T>(name)) {
            return search;
        }

        return add(std::make_unique<T>(name, args...));
    }

    template <typename T, typename... TArgs> T* find_in_owners_or_add(std::string_view name, TArgs... args) {
        if (auto search = find_in_owners<T>(name, true)) {
            return search;
        }

        return add(std::make_unique<T>(name, args...));
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

    template <typename T> bool has_any_in_children() const {
        for (auto&& child : m_children) {
            if (child->is_a<T>() || child->has_any_in_children<T>()) {
                return true;
            }
        }

        return false;
    }
};

template <typename T> T* cast(const Object* object) {
    if (object->is_a<T>()) {
        return (T*)object;
    }

    return nullptr;
}

class Typename : public Object {
public:
    Typename(std::string_view name) : Object{name} {}

    virtual const std::string get_typename() const { return m_name; }

    virtual void generate_typename_for(std::ostream& os, const Object* obj) const {
        if (auto owner_type = owner<Typename>()) {
            if (owner_type != obj->owner<Typename>()) {
                auto&& name = owner_type->get_typename();

                if (!name.empty()) {
                    owner_type->generate_typename_for(os, obj);
                    os << "::";
                }
            }
        }

        os << get_typename();
    }
};

class Type : public Typename {
public:
    Type(std::string_view name) : Typename{name} {}

    virtual size_t size() const { return m_size; }
    auto size(int size) {
        m_size = size;
        return this;
    }

    Pointer* ptr();

protected:
    size_t m_size{};
};

class Pointer : public Type {
public:
    Pointer(std::string_view name) : Type{name} {}

    auto to() const { return m_to; }
    auto to(Type* to) {
        m_to = to;
        return this;
    }

    size_t size() const override { return sizeof(uintptr_t); }
    void generate_typename_for(std::ostream& os, const Object* obj) const override {
        m_to->generate_typename_for(os, obj);
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
    Variable(std::string_view name) : Object{name} {}

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

    virtual void generate(std::ostream& os) const {
        m_type->generate_typename_for(os, this);
        os << " " << m_name << "; // 0x" << std::hex << m_offset << "\n";
    }

protected:
    Type* m_type{};
    uintptr_t m_offset{};
};

class Bitfield : public Variable {
public:
    class Field : public Object {
    public:
        Field(std::string_view name) : Object{name} {}

        auto size() const { return m_size; }
        auto size(size_t size) {
            m_size = size;
            return this;
        }

        auto offset() const { return m_offset; }
        auto offset(uintptr_t offset) {
            m_offset = offset;
            return this;
        }

        auto end() const { return offset() + size(); }

        void generate(std::ostream& os) const {
            owner<Variable>()->type()->generate_typename_for(os, this);
            os << " " << m_name << " : " << m_size << ";\n";
        }

    protected:
        size_t m_size{};
        uintptr_t m_offset{};
    };

    Bitfield(uintptr_t offset) : Variable{"bitfield_" + std::to_string(offset)} { m_offset = offset; }

    auto field(std::string_view name) { return find_or_add<Field>(name); }

    size_t size() const override {
        if (m_type == nullptr) {
            return 0;
        }

        auto alignment = m_type->size() * CHAR_BIT;
        auto max_size = m_type->size() * CHAR_BIT;

        for (auto&& child : get_all<Field>()) {
            if (child->end() > max_size) {
                max_size = ((child->end() + alignment - 1) / alignment) * alignment;
            }
        }

        return max_size / CHAR_BIT;
    }

    void generate(std::ostream& os) const override {
        if (m_type == nullptr || !has_any<Field>()) {
            return;
        }

        os << "// ";
        m_type->generate_typename_for(os, this);
        os << " " << m_name << " Offset: 0x" << std::hex << m_offset << "\n";

        std::unordered_map<uintptr_t, Field*> field_map{};

        for (auto&& field : get_all<Field>()) {
            field_map[field->offset()] = field;
        }

        auto offset = 0;
        auto max_offset = size() * CHAR_BIT;
        auto last_offset = offset;

        while (offset < max_offset) {
            if (auto search = field_map.find(offset); search != field_map.end()) {
                auto field = search->second;

                // Skip unfinished fields.
                if (field->size() == 0) {
                    ++offset;
                    continue;
                }

                if (offset - last_offset > 0) {
                    m_type->generate_typename_for(os, this);
                    os << " bitfield_pad_" << std::hex << last_offset << " : " << std::dec << offset - last_offset
                       << ";\n";
                }

                field->generate(os);
                offset += field->size();
                last_offset = offset;
            } else {
                ++offset;
            }
        }

        // Fill out the remaining space.
        if (offset - last_offset > 0) {
            m_type->generate_typename_for(os, this);
            os << " bitfield_pad_" << std::hex << last_offset << " : " << std::dec << offset - last_offset << ";\n";
        }
    }
};

class Array : public Variable {
public:
    Array(std::string_view name) : Variable{name} {}

    auto count() const { return m_count; }
    auto count(size_t size) {
        m_count = size;
        return this;
    }

    size_t size() const override { return m_type->size() * m_count; }

    void generate(std::ostream& os) const override {
        m_type->generate_typename_for(os, this);
        os << " " << m_name << "[" << std::dec << m_count << "]; // 0x" << std::hex << m_offset << "\n";
    }

protected:
    size_t m_count{};
};

class Parameter : public Object {
public:
    Parameter(std::string_view name) : Object{name} {}

    auto type() const { return m_type; }
    auto type(Type* type) {
        m_type = type;
        return this;
    }

    virtual void generate(std::ostream& os) const {
        m_type->generate_typename_for(os, this);
        os << " " << m_name;
    }

protected:
    Type* m_type{};
};

class Function : public Object {
public:
    Function(std::string_view name) : Object{name} {}

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

    virtual void generate(std::ostream& os) const {
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
            m_return_value->generate_typename_for(os, this);
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
    VirtualFunction(std::string_view name) : Function{name} {}

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
    Enum(std::string_view name) : Type{name} {}

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

    virtual void generate(std::ostream& os) const {
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
            m_type->generate_typename_for(os, this);
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
    EnumClass(std::string_view name) : Enum{name} {}

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
    Struct(std::string_view name) : Type{name} {}

    auto variable(std::string_view name) { return find_or_add_unique<Variable>(name); }
    auto bitfield(uintptr_t offset) {
        for (auto&& child : get_all<Bitfield>()) {
            if (child->offset() == offset) {
                return child;
            }
        }

        return add(std::make_unique<Bitfield>(offset));
    }

    auto array_(std::string_view name) { return find_or_add_unique<Array>(name); }
    auto struct_(std::string_view name) { return find_or_add_unique<Struct>(name); }
    auto class_(std::string_view name) { return find_or_add_unique<Class>(name); }
    auto enum_(std::string_view name) { return find_or_add_unique<Enum>(name); }
    auto enum_class(std::string_view name) { return find_or_add_unique<EnumClass>(name); }
    auto function(std::string_view name) { return find_or_add_unique<Function>(name); }
    auto virtual_function(std::string_view name) { return find_or_add_unique<VirtualFunction>(name); }

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

    virtual void generate(std::ostream& os) const {
        os << "struct " << m_name;
        generate_inheritance(os);
        os << " {\n";
        generate_internal(os);
        os << "}; // Size: 0x" << std::hex << size() << "\n";
    }

protected:
    Struct* m_parent{};

    template <typename T, typename... TArgs> T* find_or_add_unique(std::string_view name, TArgs... args) {
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
            return add(std::make_unique<T>(name, args...));
        }

        return add(std::make_unique<T>(fixed_name, args...));
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

                // Skip variables where the user has not given us a valid size (forgot to set a type or the type is
                // unfinished).
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
    Class(std::string_view name) : Struct{name} {}

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

class Namespace : public Typename {
public:
    Namespace(std::string_view name) : Typename{name} {}

    auto type(std::string_view name) { return find_in_owners_or_add<Type>(name); }
    auto struct_(std::string_view name) { return find_or_add<Struct>(name); }
    auto class_(std::string_view name) { return find_or_add<Class>(name); }
    auto enum_(std::string_view name) { return find_or_add<Enum>(name); }
    auto enum_class(std::string_view name) { return find_or_add<EnumClass>(name); }
    auto namespace_(std::string_view name) { return find_or_add<Namespace>(name); }

    virtual void generate_forward_decls(std::ostream& os) const {
        if (!has_any<Enum>() && !has_any<Struct>() && !has_any<Namespace>()) {
            return;
        }

        if (!m_name.empty()) {
            os << "namespace " << m_name << " {\n";
        }

        {
            Indent _{os};
            generate_forward_decls_internal(os);
        }

        if (!m_name.empty()) {
            os << "} // namespace " << m_name << "\n\n";
        }
    }

    virtual void generate(std::ostream& os) const {
        if (!has_any_in_children<Struct>()) {
            return;
        }

        if (!m_name.empty()) {
            os << "namespace " << m_name << " {\n";
        }

        {
            Indent _{os};
            generate_internal(os);
        }

        if (!m_name.empty()) {
            os << "} // namespace " << m_name << "\n\n";
        }
    }

protected:
    void generate_forward_decls_internal(std::ostream& os) const {
        if (has_any<Enum>()) {
            for (auto&& child : get_all<Enum>()) {
                child->generate(os);
                os << "\n";
            }
        }

        if (has_any<Struct>()) {
            for (auto&& child : get_all<Struct>()) {
                child->generate_forward_decl(os);
            }

            os << "\n";
        }

        for (auto&& ns : get_all<Namespace>()) {
            ns->generate_forward_decls(os);
        }
    }

    void generate_internal(std::ostream& os) const {
        for (auto&& child : get_all<Namespace>()) {
            child->generate(os);
        }

        if (has_any<Struct>()) {
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
    HeaderFile(std::string_view name) : Namespace{name} {}

    auto preamble(std::string_view preamble) {
        m_preamble = preamble;
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

    const std::string get_typename() const override { return ""; }

    void generate_forward_decls(std::ostream& os) const override { generate_forward_decls_internal(os); }

    void generate(std::ostream& os) const {
        if (!m_name.empty()) {
            os << "// " << m_name << "\n\n";
        }

        if (!m_preamble.empty()) {
            std::istringstream sstream{m_preamble};
            std::string line{};

            while (std::getline(sstream, line)) {
                os << "// " << line << "\n";
            }
        }

        os << "#pragma once\n\n";

        if (!m_includes.empty()) {
            for (auto&& include : m_includes) {
                os << "#include <" << include << ">\n";
            }

            os << "\n";
        }

        if (!m_local_includes.empty()) {
            for (auto&& include : m_local_includes) {
                os << "#include \"" << include << "\"\n";
            }

            os << "\n";
        }

        generate_forward_decls(os);

        auto has_structs = has_any_in_children<Struct>();

        if (has_structs) {
            os << "#pragma pack(push, 1)\n\n";
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