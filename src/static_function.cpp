#include <sdkgenny/static_function.hpp>

namespace sdkgenny {
StaticFunction::StaticFunction(std::string_view name) : Function{name} {
}

void StaticFunction::generate(std::ostream& os) const {
    os << "static ";
    generate_prototype(os);
    os << ";\n";
}
} // namespace sdkgenny