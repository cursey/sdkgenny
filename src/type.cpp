#include <sdkgenny/array.hpp>
#include <sdkgenny/pointer.hpp>
#include <sdkgenny/reference.hpp>

#include <sdkgenny/type.hpp>

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