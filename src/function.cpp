#include <sdkgenny/detail/indent.hpp>
#include <sdkgenny/parameter.hpp>
#include <sdkgenny/type.hpp>

#include <sdkgenny/function.hpp>

namespace sdkgenny {
Function::Function(std::string_view name) : Object{name} {
}

Parameter* Function::param(std::string_view name) {
    return find_or_add<Parameter>(name);
}

void Function::generate(std::ostream& os) const {
    generate_comment(os);
    generate_prototype(os);
    os << ";\n";
}

void Function::generate_source(std::ostream& os) const {
    if (m_is_defined) {
        generate_procedure(os);
    }
}

void Function::generate_prototype(std::ostream& os) const {
    if (m_return_value == nullptr) {
        os << "void";
    } else {
        m_return_value->generate_typename_for(os, this);
    }

    os << " ";
    generate_prototype_internal(os);
}

void Function::generate_prototype_internal(std::ostream& os) const {
    os << usable_name() << "(";

    auto is_first_param = true;

    for (auto&& param : get_all<Parameter>()) {
        if (is_first_param) {
            is_first_param = false;
        } else {
            os << ", ";
        }

        param->generate(os);
    }

    os << ")";
}

void Function::generate_procedure(std::ostream& os) const {
    if (m_return_value == nullptr) {
        os << "void";
    } else {
        m_return_value->generate_typename_for(os, nullptr);
    }

    os << " ";

    std::vector<const Object*> owners{};

    for (auto o = owner<Object>(); o != nullptr; o = o->owner<Object>()) {
        owners.emplace_back(o);
    }

    std::reverse(owners.begin(), owners.end());

    for (auto&& o : owners) {
        if (o->usable_name().empty()) {
            continue;
        }

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
} // namespace sdkgenny