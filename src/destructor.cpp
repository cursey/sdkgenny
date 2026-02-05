#include <algorithm>
#include <sdkgenny/destructor.hpp>
#include <sdkgenny/detail/indent.hpp>
#include <sdkgenny/parameter.hpp>

namespace sdkgenny {

Destructor::Destructor(std::string_view name) : Function{name} {
}

void Destructor::generate(std::ostream& os) const {
    generate_comment(os);

    if (m_is_virtual) {
        os << "virtual ";
    }

    os << usable_name();
    os << "(";
    auto is_first = true;
    for (auto&& param : get_all<Parameter>()) {
        if (is_first)
            is_first = false;
        else
            os << ", ";
        param->generate(os);
    }
    os << ")";

    if (m_is_virtual && m_is_pure_virtual) {
        os << " = 0;\n";
    } else {
        os << ";\n";
    }
}

void Destructor::generate_source(std::ostream& os) const {
    if (m_is_virtual && m_is_pure_virtual) {
        return;
    }

    std::vector<const Object*> owners{};
    for (auto o = owner<Object>(); o != nullptr; o = o->owner<Object>()) {
        if (o->usable_name().empty())
            break;
        owners.emplace_back(o);
    }
    std::reverse(owners.begin(), owners.end());

    for (auto&& o : owners) {
        os << o->usable_name() << "::";
    }

    generate_prototype_internal(os);

    if (m_procedure.empty()) {
        os << " {}\n";
    } else {
        os << " {\n";
        {
            detail::Indent _{os};
            os << m_procedure;
        }
        if (m_procedure.back() != '\n') {
            os << "\n";
        }
        os << "}\n";
    }
}
}