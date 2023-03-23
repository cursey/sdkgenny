#include <sdkgenny/virtual_function.hpp>

namespace sdkgenny {
VirtualFunction::VirtualFunction(std::string_view name) : Function{name} {
}

void VirtualFunction::generate(std::ostream& os) const {
    generate_comment(os);
    os << "virtual ";
    generate_prototype(os);

    if (m_procedure.empty()) {
        os << " = 0;\n";
    } else {
        os << ";\n";
    }
}
} // namespace sdkgenny
