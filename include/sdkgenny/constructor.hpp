#pragma once

#include <sdkgenny/function.hpp>
#include <string>

namespace sdkgenny {
class Constructor : public Function {
public:
    explicit Constructor(std::string_view name);


    void generate(std::ostream& os) const override;
    void generate_source(std::ostream& os) const override;

    // Support for initializer list: provide the raw text (e.g. "a(1), b(2)")
    auto initializer_list() const { return m_initializer_list; }
    auto initializer_list(std::string_view list) {
        m_initializer_list = std::string(list);
        return this;
    }

protected:
    std::string m_initializer_list{};
};
}