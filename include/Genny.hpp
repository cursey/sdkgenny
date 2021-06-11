// SdkGenny - Genny.hpp - A single file header framework for generating C++ compatible SDKs
// https://github.com/cursey/sdkgenny

#pragma once

#include <algorithm>
#include <climits>
#include <filesystem>
#include <fstream>
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

#ifdef GENNY_INCLUDE_PARSER
#include <cstdlib>
#include <optional>

#include <tao/pegtl.hpp>
#endif

namespace genny {

class Type;
class Reference;
class Pointer;
class Array;
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

    const auto& metadata() const { return m_metadata; }
    auto& metadata() { return m_metadata; }

    virtual void generate_metadata(std::ostream& os) const { 
        if (m_metadata.empty()) {
            return;
        }

        os << "// Metadata: ";

        for (auto&& md : m_metadata) {
            os << md;
            if (&md != &*m_metadata.rbegin()) {
                os << md << ", ";
            }
        }

        os << "\n";
    }

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

    template <typename T> const T* topmost_owner() const {
        const T* topmost{};

        for (auto owner = m_owner; owner != nullptr; owner = owner->m_owner) {
            if (owner->is_a<T>()) {
                topmost = (const T*)owner;
            }
        }

        return topmost;
    }

    template <typename T> T* topmost_owner() { return (T*)((const Object*)this)->topmost_owner<T>(); }

    template <typename T> std::vector<T*> owners() const {
        std::vector<T*> owners{};

        for (auto owner = m_owner; owner != nullptr; owner = owner->m_owner) {
            if (owner->is_a<T>()) {
                owners.emplace_back((T*)owner);
            }
        }

        return owners;
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

    template <typename T> void get_all_in_children(std::unordered_set<T*>& objects) const {
        if (is_a<T>()) {
            objects.emplace((T*)this);
        }

        for (auto&& child : m_children) {
            child->get_all_in_children(objects);
        }
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

    template <typename T> bool is_child_of(T* obj) const {
        for (auto&& owner : owners<T>()) {
            if (owner == obj) {
                return true;
            }
        }

        return false;
    }

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

protected:
    friend class Type;
    friend class Pointer;
    friend class Namespace;
    friend class Sdk;

    Object* m_owner{};

    std::string m_name{};
    std::vector<std::unique_ptr<Object>> m_children{};
    std::vector<std::string> m_metadata{};
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
            if (obj == nullptr || owner_type != obj->owner<Typename>()) {
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

    virtual void generate_variable_postamble(std::ostream& os) const {}

    virtual size_t size() const { return m_size; }
    auto size(int size) {
        m_size = size;
        return this;
    }

    Reference* ref();
    Pointer* ptr();
    Array* array_(size_t count = 0);

protected:
    size_t m_size{};
};

class Reference : public Type {
public:
    Reference(std::string_view name) : Type{name} {}

    auto to() const { return m_to; }
    auto to(Type* to) {
        m_to = to;
        return this;
    }

    size_t size() const override { return sizeof(uintptr_t); }

    void generate_typename_for(std::ostream& os, const Object* obj) const override {
        m_to->generate_typename_for(os, obj);
        os << "&";
    }

protected:
    Type* m_to{};
};

inline Reference* Type::ref() {
    return m_owner->find_or_add<Reference>(name() + '&')->to(this);
}

class Pointer : public Reference {
public:
    Pointer(std::string_view name) : Reference{name} {}

    auto ptr() { return m_owner->find_or_add<Pointer>(m_name + '*')->to(this); }

    void generate_typename_for(std::ostream& os, const Object* obj) const override {
        m_to->generate_typename_for(os, obj);
        os << "*";
    }
};

inline Pointer* Type::ptr() {
    return (Pointer*)m_owner->find_or_add<Pointer>(name() + '*')->to(this);
}

class Array : public Type {
public:
    Array(std::string_view name) : Type{name} {}

    auto of() const { return m_of; }
    auto of(Type* of) {
        m_of = of;
        return this;
    }

    auto count() const { return m_count; }
    auto count(size_t count) {
        m_count = count;
        return this;
    }

    size_t size() const override {
        if (m_of == nullptr) {
            return 0;
        }

        return m_of->size() * m_count;
    }

    void generate_typename_for(std::ostream& os, const Object* obj) const override {
        m_of->generate_typename_for(os, obj);
    }

    void generate_variable_postamble(std::ostream& os) const override { 
        m_of->generate_variable_postamble(os);
        os << "[" << std::dec << m_count << "]";
    }

protected:
    Type* m_of{};
    size_t m_count{};
};

inline Array* Type::array_(size_t count) {
    return (Array*)m_owner->find_or_add<Array>(name() + "[" + std::to_string(count) + "]")->of(this)->count(count);
}

class GenericType : public Type {
public:
    GenericType(std::string_view name) : Type{name} {}

    auto template_types() const { return m_template_types; }
    auto template_type(Type* type) {
        m_template_types.emplace(type);
        return this;
    }

protected:
    std::unordered_set<Type*> m_template_types{};
};

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

    // Sets the offset to be at the end of the owners current size.
    //
    // NOTE: This will increase the size of the owner by the size of this variable. If you are explicitly setting the
    // size of your structs beforehand do not use this.
    //
    // NOTE: If you're using append for the very first variables added to a struct make sure you append before setting
    // the type of the variable otherwise the variable's offset will be set after where you want it (due to itself
    // already being considered part of the struct).
    auto append() { return offset(owner<Type>()->size()); }

    virtual size_t size() const {
        if (m_type == nullptr) {
            return 0;
        }

        return m_type->size();
    }

    auto end() const { return offset() + size(); }

    virtual void generate(std::ostream& os) const {
        generate_metadata(os);
        m_type->generate_typename_for(os, this);
        os << " " << m_name;
        m_type->generate_variable_postamble(os);
        os << "; // 0x" << std::hex << m_offset << "\n";
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
            generate_metadata(os);
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

        size_t offset = 0;
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
                    os << " " << name() << "_pad_" << std::hex << last_offset << " : " << std::dec
                       << offset - last_offset << ";\n";
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
            os << " " << name() << "_pad_" << std::hex << last_offset << " : " << std::dec << offset - last_offset
               << ";\n";
        }
    }
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

    auto&& procedure() const { return m_procedure; }
    auto procedure(std::string_view procedure) {
        m_procedure = procedure;
        return this;
    }

    auto&& dependent_types() const { return m_dependent_types; }
    auto depends_on(Type* type) {
        m_dependent_types.emplace(type);
        return this;
    }

    virtual void generate(std::ostream& os) const {
        generate_prototype(os);
        os << ";\n";
    }

    virtual void generate_source(std::ostream& os) const { generate_procedure(os); }

protected:
    Type* m_return_value{};
    std::string m_procedure{};
    std::unordered_set<Type*> m_dependent_types{};

    void generate_prototype(std::ostream& os) const {
        if (m_return_value == nullptr) {
            os << "void";
        } else {
            m_return_value->generate_typename_for(os, this);
        }

        os << " ";
        generate_prototype_internal(os);
    }

    void generate_prototype_internal(std::ostream& os) const {
        os << m_name << "(";

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
        if (m_return_value == nullptr) {
            os << "void";
        } else {
            m_return_value->generate_typename_for(os, nullptr);
        }

        os << " ";

        std::vector<const Object*> owners{};

        for (auto o = owner<Object>(); o != nullptr; o = o->owner<Object>()) {
            owners.emplace_back(o);
        }

        std::reverse(owners.begin(), owners.end());

        for (auto&& o : owners) {
            if (o->name().empty()) {
                continue;
            }

            os << o->name() << "::";
        }

        generate_prototype_internal(os);

        if (m_procedure.empty()) {
            os << " {}\n";
        } else {
            os << " {\n";
            {
                Indent _{os};
                os << m_procedure;
            }
            if (m_procedure.back() != '\n') {
                os << "\n";
            }
            os << "}\n";
        }
    }
};

class VirtualFunction : public Function {
public:
    VirtualFunction(std::string_view name) : Function{name} {}

    auto vtable_index() const { return m_vtable_index; }
    auto vtable_index(int vtable_index) {
        m_vtable_index = vtable_index;
        return this;
    }

    void generate(std::ostream& os) const override {
        os << "virtual ";
        generate_prototype(os);

        if (m_procedure.empty()) {
            os << " = 0;\n";
        }
    }

protected:
    int m_vtable_index{};
};

class StaticFunction : public Function {
public:
    StaticFunction(std::string_view name) : Function{name} {}

    void generate(std::ostream& os) const override {
        os << "static ";
        generate_prototype(os);
        os << ";\n";
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

    auto struct_(std::string_view name) { return find_or_add_unique<Struct>(name); }
    auto class_(std::string_view name) { return find_or_add_unique<Class>(name); }
    auto enum_(std::string_view name) { return find_or_add_unique<Enum>(name); }
    auto enum_class(std::string_view name) { return find_or_add_unique<EnumClass>(name); }
    auto function(std::string_view name) { return find_or_add_unique<Function>(name); }
    auto virtual_function(std::string_view name) { return find_or_add_unique<VirtualFunction>(name); }
    auto static_function(std::string_view name) { return find_or_add<StaticFunction>(name); }

    auto&& parents() const { return m_parents; }
    auto parent(Struct* parent) {
        if (std::find(m_parents.begin(), m_parents.end(), parent) == m_parents.end()) {
            m_parents.emplace_back(parent);
        }

        return this;
    }

    size_t size() const override {
        size_t size = 0;

        for (auto&& parent : m_parents) {
            size += parent->size();
        }

        for (auto&& var : get_all<Variable>()) {
            auto var_end = var->end();

            if (var_end > size) {
                size = var_end;
            }
        }

        if (size == 0 && has_any<VirtualFunction>()) {
            size += sizeof(uintptr_t);
        }

        return std::max(size, m_size);
    }
    auto size(int size) {
        m_size = size;
        return this;
    }

    virtual void generate_forward_decl(std::ostream& os) const { os << "struct " << m_name << ";\n"; }

    virtual void generate(std::ostream& os) const {
        generate_metadata(os);
        os << "struct " << m_name;
        generate_inheritance(os);
        os << " {\n";
        generate_internal(os);
        os << "}; // Size: 0x" << std::hex << size() << "\n";
    }

protected:
    std::vector<Struct*> m_parents{};

    int vtable_size() const {
        auto max_index = -1;

        if (!m_parents.empty()) {
            max_index = 0;

            for (auto&& parent : m_parents) {
                if (auto parent_vtable_size = parent->vtable_size(); parent_vtable_size != -1) {
                    max_index += parent_vtable_size;
                }
            }

            if (max_index == 0) {
                max_index = -1;
            }
        }

        for (auto&& child : get_all<VirtualFunction>()) {
            max_index = std::max(max_index, child->vtable_index());
        }

        return max_index + 1;
    }

    template <typename T> T* find_in_parents(std::string_view name) {
        for (auto&& parent : m_parents) {
            if (auto obj = parent->find<T>(name)) {
                return obj;
            }
        }

        return nullptr;
    }

    template <typename T, typename... TArgs> T* find_or_add_unique(std::string_view name, TArgs... args) {
        if (auto search = find<T>(name); search != nullptr) {
            return search;
        }

        std::string fixed_name{};
        auto num_collisions = 0;
        auto has_collision = false;

        do {
            has_collision = false;

            if (find_in_parents<Object>(fixed_name.empty() ? name : fixed_name) != nullptr) {
                fixed_name = name;
                fixed_name += std::to_string(num_collisions);
                ++num_collisions;
                has_collision = true;
            }
        } while (has_collision);

        if (num_collisions == 0) {
            return add(std::make_unique<T>(name, args...));
        }

        return add(std::make_unique<T>(fixed_name, args...));
    }

    void generate_inheritance(std::ostream& os) const {
        if (m_parents.empty()) {
            return;
        }

        os << " : ";

        bool is_first = true;

        for (auto&& parent : m_parents) {
            if (is_first) {
                is_first = false;
            } else {
                os << ", ";
            }

            os << "public ";
            parent->generate_typename_for(os, this);
        }
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
        if (!m_parents.empty()) {
            offset = 0;

            for (auto&& parent : m_parents) {
                offset += parent->size();
            }
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
            // Generate normal functions normally.
            for (auto&& child : get_all<Function>()) {
                if (!child->is_a<VirtualFunction>()) {
                    child->generate(os);
                }
            }
        }

        if (has_any<VirtualFunction>()) {
            std::unordered_map<int, VirtualFunction*> vtable{};

            for (auto&& child : get_all<VirtualFunction>()) {
                auto vtable_index = child->vtable_index();

                vtable[vtable_index] = child;
            }

            auto vtable_index = 0;
            auto vtbl_size = vtable_size();

            for (; vtable_index < vtbl_size; ++vtable_index) {
                if (auto search = vtable.find(vtable_index); search != vtable.end()) {
                    search->second->generate(os);
                } else {
                    // Generate a default destructor to force addition of the vtable ptr.
                    if (vtable_index == 0) {
                        os << "virtual ~" << m_name << "() = default;\n";
                    } else {
                        os << "virtual void virtual_function_" << std::dec << vtable_index << "() = 0;\n";
                    }
                }
            }
        }
    }
};

class Class : public Struct {
public:
    Class(std::string_view name) : Struct{name} {}

    void generate_forward_decl(std::ostream& os) const override { os << "class " << m_name << ";\n"; }

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
    auto generic_type(std::string_view name) { return find_in_owners_or_add<GenericType>(name); }
    auto struct_(std::string_view name) { return find_or_add<Struct>(name); }
    auto class_(std::string_view name) { return find_or_add<Class>(name); }
    auto enum_(std::string_view name) { return find_or_add<Enum>(name); }
    auto enum_class(std::string_view name) { return find_or_add<EnumClass>(name); }
    auto namespace_(std::string_view name) { return find_or_add<Namespace>(name); }
};

class Sdk {
public:
    Sdk() = default;
    virtual ~Sdk() = default;

    auto global_ns() const { return m_global_ns.get(); }

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

    void generate(const std::filesystem::path& sdk_path) const { generate_namespace(sdk_path, m_global_ns.get()); }

protected:
    std::unique_ptr<Namespace> m_global_ns{std::make_unique<Namespace>("")};
    std::string m_preamble{};
    std::string m_postamble{};
    std::set<std::string> m_includes{};
    std::set<std::string> m_local_includes{};

    std::filesystem::path path_for_object(Object* obj) const {
        std::filesystem::path path{};
        auto owners = obj->owners<Namespace>();

        std::reverse(owners.begin(), owners.end());

        for (auto&& owner : owners) {
            if (owner->name().empty()) {
                continue;
            }

            path /= owner->name();
        }

        path /= obj->name();

        return path;
    }

    std::filesystem::path include_path_for_object(Object* obj) const {
        auto path = path_for_object(obj);
        path += ".hpp";
        return path;
    }

    std::filesystem::path source_path_for_object(Object* obj) const {
        auto path = path_for_object(obj);
        path += ".cpp";
        return path;
    }

    std::filesystem::path include_path(Object* from, Object* to) const {
        auto to_path = std::filesystem::absolute(include_path_for_object(to));
        auto from_path = std::filesystem::absolute(include_path_for_object(from));
        auto rel_path = std::filesystem::relative(to_path.parent_path(), from_path.parent_path()) / to_path.filename();
        return rel_path;
    }

    template <typename T> void generate_header(const std::filesystem::path& sdk_path, T* obj) const {
        auto obj_inc_path = sdk_path / include_path_for_object(obj);
        std::filesystem::create_directories(obj_inc_path.parent_path());
        std::ofstream os{obj_inc_path};

        if (!m_preamble.empty()) {
            std::istringstream sstream{m_preamble};
            std::string line{};

            while (std::getline(sstream, line)) {
                os << "// " << line << "\n";
            }
        }

        os << "#pragma once\n";

        for (auto&& include : m_includes) {
            os << "#include <" << include << ">\n";
        }

        for (auto&& include : m_local_includes) {
            os << "#include \"" << include << "\"\n";
        }

        std::unordered_set<Variable*> variables{};
        std::unordered_set<Function*> functions{};
        std::unordered_set<Type*> types_to_include{};
        std::unordered_set<Struct*> structs_to_forward_decl{};
        std::function<void(Type*)> add_type = [&](Type* t) {
            if (auto ref = dynamic_cast<Reference*>(t)) {
                auto to = ref->to();

                if (auto e = dynamic_cast<Enum*>(to)) {
                    types_to_include.emplace(e);
                } else if (auto s = dynamic_cast<Struct*>(to)) {
                    structs_to_forward_decl.emplace(s);
                } else {
                    add_type(to);
                }
            } else if (auto gt = dynamic_cast<GenericType*>(t)) {
                for (auto&& tt : gt->template_types()) {
                    add_type(tt);
                }
            } else if (auto e = dynamic_cast<Enum*>(t)) {
                types_to_include.emplace(e);
            } else if (auto s = dynamic_cast<Struct*>(t)) {
                types_to_include.emplace(s);
            }
        };

        obj->get_all_in_children<Variable>(variables);
        obj->get_all_in_children<Function>(functions);

        for (auto&& var : variables) {
            add_type(var->type());
        }

        for (auto&& fn : functions) {
            for (auto&& param : fn->get_all<Parameter>()) {
                add_type(param->type());
            }
            add_type(fn->returns());
        }

        if (auto s = dynamic_cast<Struct*>(obj)) {
            for (auto&& parent : s->parents()) {
                types_to_include.emplace(parent);
            }
        }

        // Go through all the types to include and replace nested types with the types they're nested within.
        for (auto it = types_to_include.begin(); it != types_to_include.end();) {
            if (auto topmost = (*it)->topmost_owner<Struct>()) {
                it = types_to_include.erase(it);

                // Skip adding the topmost owner if it's the object we're generating a header for.
                if (topmost == (Object*)obj) {
                    continue;
                }

                if (auto&& [_, was_inserted] = types_to_include.emplace(topmost); was_inserted) {
                    it = types_to_include.begin();
                }
            } else {
                ++it;
            }
        }

        for (auto&& type : types_to_include) {
            os << "#include \"" << include_path(obj, type).string() << "\"\n";
        }

        for (auto&& type : structs_to_forward_decl) {
            // Only forward decl structs we haven't already included.
            if (types_to_include.find(type) == types_to_include.end() && !type->is_child_of(obj)) {
                auto owners = type->owners<Namespace>();

                if (owners.size() > 1) {
                    std::reverse(owners.begin(), owners.end());

                    os << "namespace ";

                    for (auto&& owner : owners) {
                        if (owner->name().empty()) {
                            continue;
                        }

                        os << owner->name();

                        if (owner != owners.back()) {
                            os << "::";
                        }
                    }

                    os << " {\n";
                }

                type->generate_forward_decl(os);

                if (owners.size() > 1) {
                    os << "}\n";
                }
            }
        }

        auto owners = obj->owners<Namespace>();

        if (owners.size() > 1) {
            std::reverse(owners.begin(), owners.end());

            os << "namespace ";

            for (auto&& owner : owners) {
                if (owner->name().empty()) {
                    continue;
                }

                os << owner->name();

                if (owner != owners.back()) {
                    os << "::";
                }
            }

            os << " {\n";
        }

        os << "#pragma pack(push, 1)\n";
        obj->generate(os);
        os << "#pragma pack(pop)\n";

        if (owners.size() > 1) {
            os << "}\n";
        }

        if (!m_postamble.empty()) {
            std::istringstream sstream{m_postamble};
            std::string line{};

            while (std::getline(sstream, line)) {
                os << "// " << line << "\n";
            }
        }
    }

    template <typename T> void generate_source(const std::filesystem::path& sdk_path, T* obj) const {
        // Skip generating a source file for an object with no functions.
        if (!obj->has_any<Function>()) {
            return;
        }

        auto obj_src_path = sdk_path / source_path_for_object(obj);
        std::filesystem::create_directories(obj_src_path.parent_path());
        std::ofstream os{obj_src_path};

        if (!m_preamble.empty()) {
            std::istringstream sstream{m_preamble};
            std::string line{};

            while (std::getline(sstream, line)) {
                os << "// " << line << "\n";
            }
        }

        std::unordered_set<Variable*> variables{};
        std::unordered_set<Function*> functions{};
        std::unordered_set<Type*> types_to_include{};
        std::function<void(Type*)> add_type = [&](Type* t) {
            if (auto ref = dynamic_cast<Reference*>(t)) {
                auto to = ref->to();

                if (auto e = dynamic_cast<Enum*>(to)) {
                    types_to_include.emplace(e);
                } else if (auto s = dynamic_cast<Struct*>(to)) {
                    types_to_include.emplace(s);
                } else {
                    add_type(to);
                }
            } else if (auto gt = dynamic_cast<GenericType*>(t)) {
                for (auto&& tt : gt->template_types()) {
                    add_type(tt);
                }
            } else if (auto e = dynamic_cast<Enum*>(t)) {
                types_to_include.emplace(e);
            } else if (auto s = dynamic_cast<Struct*>(t)) {
                types_to_include.emplace(s);
            }
        };

        if (obj->is_a<Type>()) {
            add_type(obj);
        }

        obj->get_all_in_children<Function>(functions);

        for (auto&& fn : functions) {
            for (auto&& param : fn->get_all<Parameter>()) {
                add_type(param->type());
            }
            for (auto&& dependent : fn->dependent_types()) {
                add_type(dependent);
            }
            add_type(fn->returns());
        }

        for (auto&& type : types_to_include) {
            if (!type->is_child_of(obj)) {
                os << "#include \"" << include_path(obj, type).string() << "\"\n";
            }
        }

        for (auto&& fn : functions) {
            // Skip pure virtual functions.
            if (fn->is_a<VirtualFunction>() && fn->procedure().empty()) {
                continue;
            }

            fn->generate_source(os);
        }

        if (!m_postamble.empty()) {
            std::istringstream sstream{m_postamble};
            std::string line{};

            while (std::getline(sstream, line)) {
                os << "// " << line << "\n";
            }
        }
    }

    template <typename T> void generate(const std::filesystem::path& sdk_path, Namespace* ns) const {
        for (auto&& obj : ns->get_all<T>()) {
            generate_header(sdk_path, obj);
            generate_source(sdk_path, obj);
        }
    }

    void generate_namespace(const std::filesystem::path& sdk_path, Namespace* ns) const {
        generate<Enum>(sdk_path, ns);
        generate<Struct>(sdk_path, ns);

        for (auto&& child : ns->get_all<Namespace>()) {
            generate_namespace(sdk_path, child);
        }
    }
};

#ifdef GENNY_INCLUDE_PARSER
namespace parser {
using namespace tao::pegtl;

struct Comment : disable<one<'#'>, until<eolf>> {};
struct Sep : sor<one<' ', '\t'>, Comment> {};
struct Seps : star<Sep> {};

struct HexNum : seq<one<'0'>, one<'x'>, plus<xdigit>> {};
struct DecNum : plus<digit> {};
struct Num : sor<HexNum, DecNum> {};

// plus<printable characters not-including comma or closing square bracket>
struct Metadata : plus<sor<range<32, 43>, range<45, 92>, range<94, 126>>> {};
//struct Metadata : identifier {};
struct MetadataDecl : seq<two<'['>, list<Metadata, one<','>, Sep>, two<']'>> {};

struct NsId : TAO_PEGTL_STRING("namespace") {};
struct NsName : identifier {};
struct NsNameList : list<NsName, one<'.'>, Sep> {};
struct NsDecl : seq<NsId, Seps, opt<NsNameList>> {};

struct TypeId : TAO_PEGTL_STRING("type") {};
struct TypeName : identifier {};
struct TypeSize : Num {};
struct TypeDecl : seq<TypeId, Seps, TypeName, Seps, TypeSize, Seps, opt<MetadataDecl>> {};

struct EnumId : TAO_PEGTL_STRING("enum") {};
struct EnumClassId : TAO_PEGTL_STRING("class") {};
struct EnumName : identifier {};
struct EnumType : identifier {};
struct EnumDecl : seq<EnumId, Seps, opt<EnumClassId>, Seps, EnumName, Seps, opt<one<':'>, Seps, EnumType>> {};
struct EnumVal : Num {};
struct EnumValName : identifier {};
struct EnumValDecl : seq<EnumValName, Seps, one<'='>, Seps, EnumVal> {};

struct StructId : TAO_PEGTL_STRING("struct") {};
struct StructName : identifier {};
struct StructParent : identifier {};
struct StructParentList : list<StructParent, one<','>, Sep> {};
struct StructParentListDecl : seq<one<':'>, Seps, StructParentList> {};
struct StructDecl : seq<StructId, Seps, StructName, Seps, opt<StructParentListDecl>> {};

struct VarTypeNamePart : identifier {};
struct VarTypeName : list<VarTypeNamePart, one<'.'>> {};
struct VarTypePtr : one<'*'> {};
struct ArrayCount : Num {};
struct ArrayType : seq<VarTypeName, one<'['>, ArrayCount, one<']'>> {};
struct NormalVarType : seq<VarTypeName, star<VarTypePtr>> {};
struct VarType : sor<ArrayType, NormalVarType> {};
struct VarName : identifier {};
struct VarOffset : Num {};
struct VarOffsetDecl : seq<one<'@'>, Seps, VarOffset> {};
struct VarDecl : seq<VarType, Seps, VarName, Seps, opt<VarOffsetDecl>, Seps, opt<MetadataDecl>> {};

struct Decl : must<Seps, sor<NsDecl, TypeDecl, EnumDecl, EnumValDecl, StructDecl, VarDecl>, Seps> {};
struct Grammar : until<eof, sor<eolf, Decl>> {};

struct State {
    genny::Namespace* global_ns{};
    genny::Namespace* cur_ns{};
    genny::Enum* cur_enum{};
    genny::Struct* cur_struct{};

    std::vector<std::string> metadata_parts{};
    std::vector<std::string> metadata{};

    std::vector<std::string> ns_parts{};
    std::vector<std::string> ns{};

    std::string type_name{};
    size_t type_size{};

    std::string enum_name{};
    std::string enum_type{};
    bool enum_class{};
    std::string enum_val_name{};
    uint32_t enum_val{};

    std::string struct_name{};
    std::vector<std::string> struct_parents{};

    std::vector<std::string> var_type_parts{};
    std::vector<std::string> var_type{};
    int var_type_ptr{}; // The number of *'s basically.
    std::optional<size_t> array_count{};
    std::string var_name{};
    std::optional<uintptr_t> var_offset{};

    // Searches for the type identified by a vector of names. 
    template <typename T>
    T* lookup(const std::vector<std::string>& names) {
        std::function<T*(Object*, int)> search = [&](Object* parent, int i) -> T* {
            if (names.empty() || i >= names.size()) {
                return nullptr;
            }

            const auto& name = names[i];

            // Search for the name.
            auto child = parent->find<Object>(name);
            
            if (child == nullptr) {
                return nullptr;
            }

            // We found the name. Is this the type we were looking for?
            if (i == names.size() - 1) {
                return dynamic_cast<T*>(child);
            }

            return search(child, ++i);
        };

        // First search the current struct.
        if (cur_struct != nullptr) {
            if (auto type = search(cur_struct, 0)) {
                return type;
            }
        }

        // Then search the local namespace.
        if (auto type = search(cur_ns, 0)) {
            return type;
        }

        // Otherwise search the global namespace.
        return search(global_ns, 0);
    }
};

template <typename Rule> struct Action : nothing<Rule> {};

template <> struct Action<Metadata> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.metadata_parts.emplace_back(in.string_view());
    }
};

template <> struct Action<MetadataDecl> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.metadata = std::move(s.metadata_parts);
    }
};

template <> struct Action<NsName> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.ns_parts.emplace_back(in.string_view());
    }
};

template <> struct Action<NsNameList> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) { s.ns = std::move(s.ns_parts); }
};

template <> struct Action<NsDecl> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        auto cur_ns = s.global_ns;

        for (auto&& ns : s.ns) {
            cur_ns = cur_ns->namespace_(ns);
        }

        s.cur_ns = cur_ns;
        s.cur_enum = nullptr;
        s.cur_struct = nullptr;
        s.ns.clear();
    }
};

template <> struct Action<TypeSize> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.type_size = std::stoull(in.string(), nullptr, 0);
    }
};

template <> struct Action<TypeName> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.type_name = in.string_view();
    }
};

template <> struct Action<TypeDecl> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        auto type = s.cur_ns->type(s.type_name);
        type->size(s.type_size);

        if (!s.metadata.empty()) {
            type->metadata() = std::move(s.metadata);
        }

        s.type_name.clear();
        s.type_size = -1;
    }
};

template <> struct Action<EnumName> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.enum_name = in.string_view();
    }
};

template <> struct Action<EnumType> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.enum_type = in.string_view();
    }
};

template <> struct Action<EnumClassId> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) { s.enum_class = true; }
};

template <> struct Action<EnumDecl> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        if (s.enum_class) {
            s.cur_enum = s.cur_ns->enum_class(s.enum_name);
        } else {
            s.cur_enum = s.cur_ns->enum_(s.enum_name);
        }

        if (!s.enum_type.empty()) {
            s.cur_enum->type(s.cur_ns->type(s.enum_type));
        }

        s.cur_struct = nullptr;
        s.enum_name.clear();
        s.enum_type.clear();
        s.enum_class = false;
    }
};

template <> struct Action<EnumVal> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.enum_val = std::stoull(in.string(), nullptr, 0);
    }
};

template <> struct Action<EnumValName> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.enum_val_name = in.string_view();
    }
};

template <> struct Action<EnumValDecl> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        if (s.cur_enum == nullptr) {
            throw parse_error{"Cannot declare an enum value outside of an enum", in};
        }

        s.cur_enum->value(s.enum_val_name, s.enum_val);
        s.enum_val_name.clear();
        s.enum_val = 0;
    }
};

template <> struct Action<StructName> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.struct_name = in.string_view();
    }
};

template <> struct Action<StructParent> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.struct_parents.emplace_back(in.string_view());
    }
};

template <> struct Action<StructDecl> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.cur_struct = s.cur_ns->struct_(s.struct_name);

        for (auto&& parent_name : s.struct_parents) {
            auto parent = s.cur_ns->find<genny::Struct>(parent_name);

            if (parent == nullptr) {
                throw parse_error{"Parent '" + parent_name + "' does not exist", in};
            }

            s.cur_struct->parent(parent);
        }

        s.struct_name.clear();
        s.struct_parents.clear();
        s.cur_enum = nullptr;
    }
};

template <> struct Action<VarTypeNamePart> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.var_type_parts.emplace_back(in.string_view());
    }
};

template <> struct Action<VarTypeName> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.var_type = std::move(s.var_type_parts);
    }
};

template <> struct Action<VarTypePtr> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) { ++s.var_type_ptr; }
};

template <> struct Action<ArrayCount> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.array_count = std::stoull(in.string(), nullptr, 0);
    }
};

template <> struct Action<VarName> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.var_name = in.string_view();
    }
};

template <> struct Action<VarOffset> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        s.var_offset = std::stoull(in.string(), nullptr, 0);
    }
};

template <> struct Action<VarDecl> {
    template <typename ActionInput> static void apply(const ActionInput& in, State& s) {
        if (s.cur_struct == nullptr) {
            throw parse_error{"Can't declare a variable outside of a struct", in};
        }

        /*Variable* var{};

        if (s.array_count) {
            var = s.cur_struct->array_(s.var_name)->count(*s.array_count);
        } else {
            var = s.cur_struct->variable(s.var_name);
        }*/
        auto var = s.cur_struct->variable(s.var_name);

        if (s.var_offset) {
            var->offset(*s.var_offset);
        } else {
            var->append();
        }

        auto var_type = s.lookup<Type>(s.var_type);

        if (var_type == nullptr) {
            throw parse_error{"Can't find type with name '" + s.var_type.back() + "'", in};
        }

        if (s.array_count) {
            var_type = var_type->array_(*s.array_count);
        }
        
        var->type(var_type);

        for (auto i = 0; i < s.var_type_ptr; ++i) {
            var->type(var->type()->ptr());
        }

        if (!s.metadata.empty()) {
            var->metadata() = std::move(s.metadata);
        }

        s.var_type.clear();
        s.var_type_ptr = 0;
        s.array_count = std::nullopt;
        s.var_name.clear();
        s.var_offset = std::nullopt;
    }
};
} // namespace parser
#endif

} // namespace genny