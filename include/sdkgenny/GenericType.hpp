#pragma once

#include <unordered_set>

#include "Type.hpp"

namespace sdkgenny {
class GenericType : public Type {
public:
    explicit GenericType(std::string_view name);

    auto&& template_types() const { return m_template_types; }
    auto template_type(Type* type) {
        m_template_types.emplace(type);
        return this;
    }

protected:
    std::unordered_set<Type*> m_template_types{};
};
} // namespace sdkgenny