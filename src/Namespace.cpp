#include "sdkgenny/Class.hpp"
#include "sdkgenny/Enum.hpp"
#include "sdkgenny/EnumClass.hpp"
#include "sdkgenny/GenericType.hpp"
#include "sdkgenny/Struct.hpp"

#include "sdkgenny/Namespace.hpp"

namespace sdkgenny {

Namespace::Namespace(std::string_view name) : Typename{name} {
}

Type* Namespace::type(std::string_view name) {
    return find_in_owners_or_add<Type>(name);
}
GenericType* Namespace::generic_type(std::string_view name) {
    return find_in_owners_or_add<GenericType>(name);
}
Struct* Namespace::struct_(std::string_view name) {
    return find_or_add<Struct>(name);
}
Class* Namespace::class_(std::string_view name) {
    return find_or_add<Class>(name);
}
Enum* Namespace::enum_(std::string_view name) {
    return find_or_add<Enum>(name);
}
EnumClass* Namespace::enum_class(std::string_view name) {
    return find_or_add<EnumClass>(name);
}
Namespace* Namespace::namespace_(std::string_view name) {
    return find_or_add<Namespace>(name);
}
} // namespace sdkgenny