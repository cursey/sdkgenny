#pragma once

#include <sdkgenny/type.hpp>

namespace sdkgenny {
class Reference : public Type {
public:
    explicit Reference(std::string_view name);

    auto to() const { return m_to; }
    auto to(Type* to) {
        m_to = to;
        return this;
    }

    size_t size() const override { return sizeof(uintptr_t); }

    void generate_typename_for(std::ostream& os, const Object* obj) const override;

protected:
    Type* m_to{};
};
} // namespace sdkgenny