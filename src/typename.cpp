#include <sdkgenny/typename.hpp>

namespace sdkgenny {
Typename::Typename(std::string_view name) : Object{name} {
}

void Typename::generate_typename_for(std::ostream& os, const Object* obj) const {
    if (m_simple_typename_generation) {
        os << usable_name();
        return;
    }

    if (auto owner_type = owner<Typename>()) {
        if (obj == nullptr || owner_type != obj->owner<Typename>()) {
            auto&& name = owner_type->name();

            if (!name.empty()) {
                owner_type->generate_typename_for(os, obj);
                os << "::";
            }
        }
    }

    os << usable_name();
}

} // namespace sdkgenny