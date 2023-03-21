#pragma once

#include <sdkgenny/object.hpp>

namespace sdkgenny {
class Typename : public Object {
public:
    explicit Typename(std::string_view name);

    virtual void generate_typename_for(std::ostream& os, const Object* obj) const;

    bool simple_typename_generation() const { return m_simple_typename_generation; }
    Typename* simple_typename_generation(bool simple_generation) {
        m_simple_typename_generation = simple_generation;
        return this;
    }

protected:
    bool m_simple_typename_generation{};
};
} // namespace sdkgenny