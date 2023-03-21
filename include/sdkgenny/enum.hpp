#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#include <sdkgenny/type.hpp>

namespace sdkgenny {
class Enum : public Type {
public:
    explicit Enum(std::string_view name);

    Enum* value(std::string_view name, uint64_t value);

    auto type() const { return m_type; }
    auto type(Type* type) {
        m_type = type;
        return this;
    }

    auto&& values() const { return m_values; }
    auto&& values() { return m_values; }

    size_t size() const override;

    virtual void generate_forward_decl(std::ostream& os) const;
    virtual void generate(std::ostream& os) const;

protected:
    std::vector<std::tuple<std::string, uint64_t>> m_values{};
    Type* m_type{};

    void generate_type(std::ostream& os) const;
    void generate_enums(std::ostream& os) const;
};
} // namespace sdkgenny