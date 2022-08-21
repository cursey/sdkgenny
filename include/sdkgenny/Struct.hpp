#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <ostream>
#include <string>
#include <unordered_set>

#include "Type.hpp"

namespace sdkgenny {
class Variable;
class Constant;
class Class;
class Enum;
class EnumClass;
class Function;
class VirtualFunction;
class StaticFunction;

class Struct : public Type {
public:
    explicit Struct(std::string_view name);

    Variable* variable(std::string_view name);
    Constant* constant(std::string_view name);

    // Returns a map of bit_offset, bitfield_variable at a given offset. Optionally, it will ignore a given variable
    // while constructing the map.
    std::map<uintptr_t, Variable*> bitfield(uintptr_t offset, Variable* ignore = nullptr) const;

    Struct* struct_(std::string_view name);
    Class* class_(std::string_view name);
    Enum* enum_(std::string_view name);
    EnumClass* enum_class(std::string_view name);
    Function* function(std::string_view name);
    VirtualFunction* virtual_function(std::string_view name);
    StaticFunction* static_function(std::string_view name);

    const std::vector<Struct*>& parents() const;
    Struct* parent(Struct* parent);

    size_t size() const override;
    auto size(int size) {
        m_size = size;
        return this;
    }

    virtual void generate_forward_decl(std::ostream& os) const;
    virtual void generate(std::ostream& os) const;

    struct Dependencies {
        std::unordered_set<Type*> hard{};
        std::unordered_set<Type*> soft{};
    };

    Dependencies dependencies();

protected:
    std::vector<Struct*> m_parents{};

    int vtable_size() const;

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

    void generate_inheritance(std::ostream& os) const;
    void generate_bitfield(std::ostream& os, uintptr_t offset) const;
    void generate_internal(std::ostream& os) const;
};
} // namespace sdkgenny