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
#include <sdkgenny/pointer.hpp>
#include <sdkgenny/template_parameter.hpp>

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

TemplateParameter* Struct::template_parameter(std::string_view name) {
    if (auto existing = find<TemplateParameter>(name)) {
        return existing;
    }
    auto param = add(std::make_unique<TemplateParameter>(name));
    m_template_params.emplace_back(param);
    return param;
}

const std::vector<TemplateParameter*>& Struct::template_parameters() const {
    return m_template_params;
}

bool Struct::is_template() const {
    return !m_template_params.empty();
}

static Type* substitute_type(Type* type, const std::unordered_map<TemplateParameter*, Type*>& subst) {
    if (auto tp = dynamic_cast<TemplateParameter*>(type)) {
        auto it = subst.find(tp);
        return it != subst.end() ? it->second : type;
    }
    if (auto ptr = dynamic_cast<Pointer*>(type)) {
        auto new_to = substitute_type(ptr->to(), subst);
        return new_to != ptr->to() ? new_to->ptr() : type;
    }
    if (auto ref = dynamic_cast<Reference*>(type)) {
        auto new_to = substitute_type(ref->to(), subst);
        return new_to != ref->to() ? new_to->ref() : type;
    }
    if (auto arr = dynamic_cast<Array*>(type)) {
        auto new_of = substitute_type(arr->of(), subst);
        return new_of != arr->of() ? new_of->array_(arr->count()) : type;
    }
    return type;
}

Struct* Struct::instantiate(const std::vector<Type*>& args) const {
    if (args.size() != m_template_params.size()) {
        return nullptr;
    }

    // Build substitution map
    std::unordered_map<TemplateParameter*, Type*> subst;
    for (size_t i = 0; i < m_template_params.size(); ++i) {
        subst[m_template_params[i]] = args[i];
    }

    // Build instantiated name: "Foo<int, float*>"
    std::string inst_name = std::string{name()} + "<";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) inst_name += ", ";
        inst_name += args[i]->name();
    }
    inst_name += ">";

    // Create the instantiated struct in the same owner
    auto owner = m_owner;
    if (!owner) return nullptr;

    // Check if already instantiated
    if (auto existing = owner->find<Struct>(inst_name)) {
        return existing;
    }

    // Preserve the dynamic type (Class vs Struct)
    std::unique_ptr<Struct> instantiated;
    if (dynamic_cast<const Class*>(this) != nullptr) {
        instantiated = std::make_unique<Class>(inst_name);
    } else {
        instantiated = std::make_unique<Struct>(inst_name);
    }
    auto inst = owner->add(std::move(instantiated));

    // Allow <>, in the usable name
    inst->usable_name = [inst] {
        std::string result{};
        constexpr auto allowed_chars = "*&[]:,<> ";
        for (auto&& c : inst->name()) {
            auto cc = static_cast<unsigned char>(c);
            if (!std::isalnum(cc) && std::strchr(allowed_chars, cc) == nullptr) {
                result += '_';
            } else {
                result += c;
            }
        }
        if (!result.empty() && isdigit(result[0])) {
            result = "_" + result;
        }
        return result;
    };
    inst->usable_name_decl = inst->usable_name;

    // Copy explicit size
    if (m_size > 0) {
        inst->size(static_cast<int>(m_size));
    }

    // Copy parents with type substitution
    for (auto parent : m_parents) {
        inst->parent(parent);
    }

    // Clone variables with type substitution.
    // Offsets from the template may be unreliable when computed after a TemplateParameter
    // field (size 0). We track whether any preceding variable had a size-0 type;
    // once that happens, all subsequent auto-computed offsets (including + delta) are
    // tainted and must be re-appended. Only truly pinned @ offsets that appear BEFORE
    // any size-0 predecessor are preserved.
    bool offsets_tainted = false;
    for (auto var : get_all<Variable>()) {
        if (var->type()->size() == 0) {
            offsets_tainted = true;
        }

        auto new_type = substitute_type(var->type(), subst);
        auto new_var = inst->variable(var->name());
        new_var->type(new_type);

        if (var->offset_is_explicit() && !offsets_tainted) {
            new_var->offset(var->offset());
        } else {
            new_var->append();
            // Re-apply stored delta (from + N syntax)
            if (var->delta() > 0) {
                new_var->offset(new_var->offset() + var->delta());
            }
        }

        if (var->is_bitfield()) {
            new_var->bit_size(var->bit_size());
            new_var->bit_offset(var->bit_offset());
        }

        if (!var->metadata().empty()) {
            new_var->metadata() = var->metadata();
        }
    }

    // Instantiated structs don't generate their own header — the C++ template handles it
    inst->skip_generation(true);

    return inst;
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
    if (is_template()) {
        os << "template<";
        bool first = true;
        for (auto param : m_template_params) {
            if (!first) os << ", ";
            first = false;
            os << "typename " << param->name();
        }
        os << "> ";
    }
    os << "struct " << usable_name_decl() << ";\n";
}

void Struct::generate(std::ostream& os) const {
    generate_comment(os);
    generate_metadata(os);

    // Emit template parameter list if this is a template struct
    if (is_template()) {
        os << "template<";
        bool first = true;
        for (auto param : m_template_params) {
            if (!first) os << ", ";
            first = false;
            os << "typename " << param->name();
        }
        os << ">\n";
    }

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
    uintptr_t last_bit = 0;
    Type* bitfield_type{};

    for (auto&& [bit_offset, var] : bitfield(offset)) {
        if (bit_offset - last_bit > 0) {
            os << "private: ";
            var->type()->generate_typename_for(os, var);
            os << " pad_bitfield_" << std::hex << offset << "_" << std::hex << last_bit << " : " << std::dec
               << bit_offset - last_bit << "; public:\n";
        }

        var->generate(os);
        last_bit = bit_offset + var->bit_size();
        bitfield_type = var->type();
    }

    // Fill out the remaining space in the bitfield if necessary.
    auto num_bits = bitfield_type->size() * CHAR_BIT;

    if (last_bit != num_bits) {
        auto bit_offset = num_bits;

        os << "private: ";
        bitfield_type->generate_typename_for(os, nullptr);
        os << " pad_bitfield_" << std::hex << offset << "_" << std::hex << last_bit << " : " << std::dec
           << bit_offset - last_bit << "; public:\n";
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

    // For template structs, emit variables in declaration order.
    // TemplateParameter types have size 0, so the offset-based padding loop can't work.
    // We still honor explicit @ offsets by emitting padding before pinned fields.
    if (is_template()) {
        size_t current_offset = 0;

        for (auto&& var : get_all<Variable>()) {
            // Emit padding before variables with explicit @ offsets
            if (var->offset_is_explicit() && var->offset() > current_offset) {
                os << "private: char pad_" << std::hex << current_offset
                   << "[0x" << std::hex << var->offset() - current_offset
                   << "]; public:\n";
                current_offset = var->offset();
            }

            var->generate(os);
            current_offset += var->size();
        }

        // Trailing padding to fill explicit struct size
        if (m_size > current_offset) {
            os << "private: char pad_" << std::hex << current_offset
               << "[0x" << std::hex << m_size - current_offset
               << "]; public:\n";
        }
    } else {
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
                    os << "private: char pad_" << std::hex << last_offset << "[0x" << std::hex << offset - last_offset
                       << "]; public:\n";
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
            os << "private: char pad_" << std::hex << last_offset << "[0x" << std::hex << offset - last_offset
               << "]; public:\n";
        }
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
                    os << "private: virtual void virtual_function_" << std::dec << vtable_index << "() = 0; public:\n";
                }
            }
        }
    }
}
} // namespace sdkgenny