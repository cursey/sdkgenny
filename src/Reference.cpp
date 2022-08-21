#include "sdkgenny/Reference.hpp"

namespace sdkgenny {
Reference::Reference(std::string_view name) : Type{name} {
}

void Reference::generate_typename_for(std::ostream& os, const Object* obj) const {
    m_to->generate_typename_for(os, obj);
    os << "&";
}
} // namespace sdkgenny