#include <sdkgenny/array.hpp>

namespace sdkgenny {
Array::Array(std::string_view name) : Type{name} {
}

Array* Array::count(size_t count) {
    // Fix the name of this array type.
    if (m_of != nullptr && count != m_count) {
        auto& base = m_of->name();
        auto first_brace = base.find_first_of('[');
        auto head = base.substr(0, first_brace);
        std::string tail{};

        if (first_brace != std::string::npos) {
            tail = base.substr(first_brace);
        }

        m_name = head + '[' + std::to_string(count) + ']' + tail;
    }

    m_count = count;

    return this;
}

size_t Array::size() const {
    if (m_of == nullptr) {
        return 0;
    }

    return m_of->size() * m_count;
}

void Array::generate_typename_for(std::ostream& os, const Object* obj) const {
    m_of->generate_typename_for(os, obj);
}

void Array::generate_variable_postamble(std::ostream& os) const {
    os << "[" << std::dec << m_count << "]";
    m_of->generate_variable_postamble(os);
}
} // namespace sdkgenny