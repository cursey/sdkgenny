#include <algorithm>
#include <sdkgenny/constructor.hpp>
#include <sdkgenny/detail/indent.hpp>

namespace sdkgenny {

Constructor::Constructor(std::string_view name) : Function{name} {
}

void Constructor::generate(std::ostream& os) const {
    generate_comment(os);
    generate_prototype_internal(os);
    os << ";\n";
}

void Constructor::generate_source(std::ostream& os) const {
    // (namespaces / enclosing types)
    std::vector<const Object*> owners{};
    for (auto o = owner<Object>(); o != nullptr; o = o->owner<Object>()) {
        if (o->usable_name().empty()) {
            break;
        }
        owners.emplace_back(o);
    }
    std::reverse(owners.begin(), owners.end());

    for (auto&& o : owners) {
        os << o->usable_name() << "::";
    }
    
    generate_prototype_internal(os);


    if (!m_initializer_list.empty()) {
        os << " : " << m_initializer_list;
    }

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