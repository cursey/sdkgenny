#pragma once

#include <sdkgenny/function.hpp>

namespace sdkgenny {
class Destructor : public Function {
public:
    explicit Destructor(std::string_view name);
    void generate(std::ostream& os) const override;
    void generate_source(std::ostream& os) const override;

    auto is_virtual() const { return m_is_virtual; }
    auto is_virtual(bool v) {
        m_is_virtual = v;
        return this;
    }
    auto is_pure_virtual() const { return m_is_pure_virtual; }
    auto is_pure_virtual(bool pv) {
        m_is_pure_virtual = pv;
        return this;
    }

protected:
    bool m_is_virtual{};
    bool m_is_pure_virtual{};
};
}