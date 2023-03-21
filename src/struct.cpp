#include <climits>
#include <unordered_map>

#include <sdkgenny/array.hpp>
#include <sdkgenny/class.hpp>
#include <sdkgenny/constant.hpp>
#include <sdkgenny/detail/indent.hpp>
#include <sdkgenny/enum.hpp>
#include <sdkgenny/enum_class.hpp>
#include <sdkgenny/function.hpp>
#include <sdkgenny/generic_type.hpp>
#include <sdkgenny/parameter.hpp>
#include <sdkgenny/reference.hpp>
#include <sdkgenny/static_function.hpp>
#include <sdkgenny/variable.hpp>
#include <sdkgenny/virtual_function.hpp>

#include <sdkgenny/struct.hpp>

namespace sdkgenny {
Struct::Struct(std::string_view name) : Type{name} {
}

Variable* Struct::variable(std::string_view name) {
    return find_or_add_unique<Variable>(name);
}

Constant* Struct::constant(std::string_view name) {
    return find_or_add_unique<Constant>(name);
}

std::map<uintptr_t, Variable*> Struct::bitfield(uintptr_t offset, Variable* ignore) const {
    std::map<uintptr_t, Variable*> vars{};

    for (auto&& child : m_children) {
        if (auto var = dynamic_cast<Variable*>(child.get()); var != nullptr && var != ignore) {
            if (var->offset() == offset) {
                vars[var->bit_offset()] = var;
            }
        }
    }

    return vars;
}

Struct* Struct::struct_(std::string_view name) {
    return find_or_add_unique<Struct>(name);
}
Class* Struct::class_(std::string_view name) {
    return find_or_add_unique<Class>(name);
}
Enum* Struct::enum_(std::string_view name) {
    return find_or_add_unique<Enum>(name);
}
EnumClass* Struct::enum_class(std::string_view name) {
    return find_or_add_unique<EnumClass>(name);
}
Function* Struct::function(std::string_view name) {
    return find_or_add_unique<Function>(name);
}
VirtualFunction* Struct::virtual_function(std::string_view name) {
    return find_or_add_unique<VirtualFunction>(name);
}
StaticFunction* Struct::static_function(std::string_view name) {
    return find_or_add<StaticFunction>(name);
}

const std::vector<Struct*>& Struct::parents() const {
    return m_parents;
}

Struct* Struct::parent(Struct* parent) {
    if (std::find(m_parents.begin(), m_parents.end(), parent) == m_parents.end()) {
        m_parents.emplace_back(parent);
    }

    return this;
}

size_t Struct::size() const {
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

    return std::max<size_t>(size, m_size);
}

void Struct::generate_forward_decl(std::ostream& os) const {
    os << "struct " << usable_name_decl() << ";\n";
}

void Struct::generate(std::ostream& os) const {
    generate_metadata(os);
    os << "struct " << usable_name_decl();
    generate_inheritance(os);
    os << " {\n";
    generate_internal(os);
    os << "}; // Size: 0x" << std::hex << size() << "\n";
}

Struct::Dependencies Struct::dependencies() {
    Dependencies deps{};

    std::function<void(Object*)> add_dep{};
    std::function<void(Object*)> add_hard_dep = [&](Object* obj) {
        if (obj == nullptr || obj == this || obj->is_child_of(this)) {
            return;
        }

        if (auto parent = obj->direct_owner(); parent != nullptr && parent->is_a<Struct>()) {
            // Structs declared within structs need their parent struct to be a hard dependency.
            add_hard_dep(parent);
        } else if (obj->is_a<Struct>() || obj->is_a<Enum>()) {
            deps.hard.emplace(dynamic_cast<Type*>(obj));
        }
    };
    std::function<void(Object*)> add_soft_dep = [&](Object* obj) {
        if (obj == nullptr || obj == this || obj->is_child_of(this)) {
            return;
        }

        if (auto ref = dynamic_cast<Reference*>(obj)) {
            add_soft_dep(ref->to());
        } else if (obj->is_a<Struct>() || obj->is_a<Enum>() || obj->is_a<GenericType>()) {
            if (obj->is_a<Enum>()) {
                // Enums are always hard dependencies.
                add_hard_dep(obj);
            } else if (obj->is_a<GenericType>()) {
                // GenericTypes may have hard or soft dependencies as template types.
                add_dep(obj);
            } else if (auto parent = obj->direct_owner(); parent != nullptr && parent->is_a<Struct>()) {
                // Structs declared within structs need their parent struct to be a hard dependency.
                add_hard_dep(parent);
            } else {
                deps.soft.emplace(dynamic_cast<Type*>(obj));
            }
        }
    };
    add_dep = [&](Object* obj) {
        if (auto arr = dynamic_cast<Array*>(obj)) {
            add_dep(arr->of());
        } else if (auto gt = dynamic_cast<GenericType*>(obj)) {
            for (auto&& type : gt->template_types()) {
                add_dep(type);
            }
        } else if (auto ref = dynamic_cast<Reference*>(obj)) {
            add_soft_dep(ref->to());
        } else {
            add_hard_dep(obj);
        }
    };

    for (auto&& parent : parents()) {
        add_hard_dep(parent);
    }

    for (auto&& var : get_all<Variable>()) {
        add_dep(var->type());
    }

    for (auto&& var : get_all<Constant>()) {
        add_dep(var->type());
    }

    for (auto&& fn : get_all<Function>()) {
        for (auto&& param : fn->get_all<Parameter>()) {
            add_dep(param->type());
        }

        add_dep(fn->returns());
    }

    for (auto&& s : get_all<Struct>()) {
        auto s_deps = s->dependencies();

        for (auto&& dep : s_deps.hard) {
            add_hard_dep(dep);
        }

        for (auto&& dep : s_deps.soft) {
            add_soft_dep(dep);
        }
    }

    // If a type is both a hard and soft dependency, remove it from the soft dependencies.
    for (auto&& dep : deps.hard) {
        if (deps.soft.find(dep) != deps.soft.end()) {
            deps.soft.erase(dep);
        }
    }

    return deps;
}

int Struct::vtable_size() const {
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
        max_index = std::max<int>(max_index, child->vtable_index());
    }

    return max_index + 1;
}

void Struct::generate_inheritance(std::ostream& os) const {
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

void Struct::generate_bitfield(std::ostream& os, uintptr_t offset) const {
    auto last_bit = 0;
    Type* bitfield_type{};

    for (auto&& [bit_offset, var] : bitfield(offset)) {
        if (bit_offset - last_bit > 0) {
            var->type()->generate_typename_for(os, var);
            os << " pad_bitfield_" << std::hex << offset << "_" << std::hex << last_bit << " : " << std::dec
               << bit_offset - last_bit << ";\n";
        }

        var->generate(os);
        last_bit = bit_offset + var->bit_size();
        bitfield_type = var->type();
    }

    // Fill out the remaining space in the bitfield if necessary.
    auto num_bits = bitfield_type->size() * CHAR_BIT;

    if (last_bit != num_bits) {
        auto bit_offset = num_bits;

        bitfield_type->generate_typename_for(os, nullptr);
        os << " pad_bitfield_" << std::hex << offset << "_" << std::hex << last_bit << " : " << std::dec
           << bit_offset - last_bit << ";\n";
    }
}

void Struct::generate_internal(std::ostream& os) const {
    detail::Indent _{os};

    for (auto&& child : get_all<Enum>()) {
        child->generate(os);
        os << "\n";
    }

    for (auto&& child : get_all<Struct>()) {
        child->generate(os);
        os << "\n";
    }

    for (auto&& child : get_all<Constant>()) {
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

            if (var->is_bitfield()) {
                generate_bitfield(os, offset);
            } else {
                var->generate(os);
            }

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
                    os << "virtual ~" << usable_name() << "() = default;\n";
                } else {
                    os << "virtual void virtual_function_" << std::dec << vtable_index << "() = 0;\n";
                }
            }
        }
    }
}
} // namespace sdkgenny