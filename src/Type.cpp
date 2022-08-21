#include "sdkgenny/Array.hpp"
#include "sdkgenny/Pointer.hpp"
#include "sdkgenny/Reference.hpp"

#include "sdkgenny/Type.hpp"

namespace sdkgenny {
Type::Type(std::string_view name) : Typename{name} {
}

Reference* Type::ref() {
    return m_owner->find_or_add<Reference>(name() + '&')->to(this);
}

Pointer* Type::ptr() {
    return (Pointer*)m_owner->find_or_add<Pointer>(name() + '*')->to(this);
}

Array* Type::array_(size_t count) {
    return m_owner->find_or_add<Array>(name() + "[0]")->of(this)->count(count);
}
} // namespace sdkgenny