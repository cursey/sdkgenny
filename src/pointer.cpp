#include <sdkgenny/pointer.hpp>

namespace sdkgenny {
Pointer::Pointer(std::string_view name) : Reference{name} {
}

void Pointer::generate_typename_for(std::ostream& os, const Object* obj) const {
    m_to->generate_typename_for(os, obj);
    os << "*";
}
} // namespace sdkgenny