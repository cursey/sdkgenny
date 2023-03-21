#pragma once

#include <cstdint>

#include <sdkgenny/function.hpp>

namespace sdkgenny {
class VirtualFunction : public Function {
public:
    explicit VirtualFunction(std::string_view name);

    auto vtable_index() const { return m_vtable_index; }
    auto vtable_index(uint32_t vtable_index) {
        m_vtable_index = vtable_index;
        return this;
    }

    void generate(std::ostream& os) const override;

protected:
    uint32_t m_vtable_index{};
};
} // namespace sdkgenny