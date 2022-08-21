#include "sdkgenny/detail/Indent.hpp"

#include "sdkgenny/Enum.hpp"

namespace sdkgenny {
Enum::Enum(std::string_view name) : Type{name} {
}

Enum* Enum::value(std::string_view name, uint64_t value) {
    for (auto&& [val_name, val_val] : m_values) {
        if (val_name == name) {
            val_val = value;
            return this;
        }
    }

    m_values.emplace_back(name, value);
    return this;
}

size_t Enum::size() const {
    if (m_type == nullptr) {
        return sizeof(int);
    } else {
        return m_type->size();
    }
}

void Enum::generate_forward_decl(std::ostream& os) const {
    os << "enum " << usable_name_decl() << ";\n";
}

void Enum::generate(std::ostream& os) const {
    os << "enum " << usable_name_decl();
    generate_type(os);
    os << " {\n";
    generate_enums(os);
    os << "};\n";
}

void Enum::generate_type(std::ostream& os) const {
    if (m_type != nullptr) {
        os << " : ";
        m_type->generate_typename_for(os, this);
    }
}

void Enum::generate_enums(std::ostream& os) const {
    detail::Indent _{os};

    for (auto&& [name, value] : m_values) {
        if (!m_type || 1ull << (m_type->size() * 8) > value)
            os << name << " = " << value << ",\n";
    }
}
} // namespace sdkgenny