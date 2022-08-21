#include "sdkgenny/Type.hpp"

#include "sdkgenny/Parameter.hpp"

namespace sdkgenny {
Parameter::Parameter(std::string_view name) : Object{name} {
}

void Parameter::generate(std::ostream& os) const {
    m_type->generate_typename_for(os, this);
    os << " " << usable_name();
}
} // namespace sdkgenny