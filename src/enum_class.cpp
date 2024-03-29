#include <sdkgenny/enum_class.hpp>

namespace sdkgenny {
EnumClass::EnumClass(std::string_view name) : Enum{name} {
}

void EnumClass::generate_forward_decl(std::ostream& os) const {
    os << "enum class " << usable_name_decl() << ";\n";
}
void EnumClass::generate(std::ostream& os) const {
    generate_comment(os);
    os << "enum class " << usable_name_decl();
    generate_type(os);
    os << " {\n";
    generate_enums(os);
    os << "};\n";
}
} // namespace sdkgenny