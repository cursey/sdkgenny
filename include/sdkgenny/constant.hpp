#pragma once

#include <concepts>
#include <ostream>
#include <string>

#include <sdkgenny/object.hpp>

namespace sdkgenny {
class Type;

class Constant : public Object {
public:
    explicit Constant(std::string_view name);

    auto type() const { return m_type; }
    auto type(Type* type) {
        m_type = type;
        return this;
    }

    // Helper that recurses though owners to find the correct type.
    Constant* type(std::string_view name);

    const auto& value() const { return m_value; }
    auto value(std::string_view value) {
        m_value = value;
        return this;
    }

    template <std::floating_point T> auto real(T value) {
        m_value = std::to_string(value);
        return this;
    }

    template <std::integral T> auto integer(T value) {
        m_value = std::to_string(value);
        return this;
    }

    Constant* string(const std::string& value);

    virtual void generate(std::ostream& os) const;

protected:
    Type* m_type{};
    std::string m_value{};
};
} // namespace sdkgenny