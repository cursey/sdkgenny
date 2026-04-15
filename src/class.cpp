#include <sdkgenny/class.hpp>
#include <sdkgenny/template_parameter.hpp>

namespace sdkgenny {
Class::Class(std::string_view name) : Struct{name} {
}

void Class::generate_forward_decl(std::ostream& os) const {
    if (is_template()) {
        os << "template<";
        bool first = true;
        for (auto param : m_template_params) {
            if (!first) os << ", ";
            first = false;
            os << "typename " << param->name();
        }
        os << "> ";
    }
    os << "class " << usable_name_decl() << ";\n";
}

void Class::generate(std::ostream& os) const {
    generate_comment(os);
    if (is_template()) {
        os << "template<";
        bool first = true;
        for (auto param : m_template_params) {
            if (!first) os << ", ";
            first = false;
            os << "typename " << param->name();
        }
        os << ">\n";
    }
    os << "class " << usable_name_decl();
    generate_inheritance(os);
    os << " {\n";

    if (!m_children.empty()) {
        os << "public:\n";
    }

    generate_internal(os);

    os << "}; // Size: 0x" << std::hex << size() << "\n";
}
} // namespace sdkgenny
