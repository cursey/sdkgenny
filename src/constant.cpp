#include <sdkgenny/type.hpp>

#include <sdkgenny/constant.hpp>

namespace sdkgenny {
Constant::Constant(std::string_view name) : Object{name} {
}

Constant* Constant::type(std::string_view name) {
    m_type = find_in_owners_or_add<Type>(name);
    return this;
}

Constant* Constant::string(const std::string& value) {
    m_value = "\"" + value + "\"";
    return this;
}

void Constant::generate(std::ostream& os) const {
    os << "static constexpr ";
    generate_metadata(os);
    m_type->generate_typename_for(os, this);
    os << " " << usable_name();
    m_type->generate_variable_postamble(os);
    os << " = " << m_value << ";";
}
} // namespace sdkgenny