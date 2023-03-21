#pragma once

#include <ostream>

#include <sdkgenny/object.hpp>

namespace sdkgenny {
class Type;

class Parameter : public Object {
public:
    explicit Parameter(std::string_view name);

    auto type() const { return m_type; }
    auto type(Type* type) {
        m_type = type;
        return this;
    }

    virtual void generate(std::ostream& os) const;

protected:
    Type* m_type{};
};
} // namespace sdkgenny