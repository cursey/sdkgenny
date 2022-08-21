#pragma once

#include <cstddef>

#include "Type.hpp"

namespace sdkgenny {
class Array : public Type {
public:
    explicit Array(std::string_view name);

    auto of() const { return m_of; }
    auto of(Type* of) {
        m_of = of;
        return this;
    }

    auto count() const { return m_count; }
    Array* count(size_t count);

    size_t size() const override;

    void generate_typename_for(std::ostream& os, const Object* obj) const override;
    void generate_variable_postamble(std::ostream& os) const override;

protected:
    Type* m_of{};
    size_t m_count{};
};
} // namespace sdkgenny