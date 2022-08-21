#pragma once

#include <ostream>
#include <string>
#include <unordered_set>

#include "Object.hpp"

namespace sdkgenny {
class Type;
class Parameter;

class Function : public Object {
public:
    explicit Function(std::string_view name);

    Parameter* param(std::string_view name);

    auto returns() const { return m_return_value; }
    auto returns(Type* return_value) {
        m_return_value = return_value;
        return this;
    }

    auto&& procedure() const { return m_procedure; }
    auto procedure(std::string_view procedure) {
        m_procedure = procedure;
        return this;
    }

    auto&& dependencies() const { return m_dependencies; }
    auto depends_on(Type* type) {
        m_dependencies.emplace(type);
        return this;
    }

    auto&& defined() const { return m_is_defined; }
    auto defined(bool is_defined) {
        m_is_defined = is_defined;
        return this;
    }

    virtual void generate(std::ostream& os) const;
    virtual void generate_source(std::ostream& os) const;

protected:
    Type* m_return_value{};
    std::string m_procedure{};
    std::unordered_set<Type*> m_dependencies{};
    bool m_is_defined{true};

    void generate_prototype(std::ostream& os) const;
    void generate_prototype_internal(std::ostream& os) const;
    void generate_procedure(std::ostream& os) const;
};
} // namespace sdkgenny