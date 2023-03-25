#pragma once

#include <cstddef>
#include <ostream>
#include <string_view>

#include <sdkgenny/typename.hpp>

namespace sdkgenny {
class Reference;
class Pointer;
class Array;

class Type : public Typename {
public:
    explicit Type(std::string_view name);

    virtual void generate_variable_postamble(std::ostream& os [[maybe_unused]]) const {}

    virtual size_t size() const { return m_size; }
    auto size(int size) {
        m_size = size;
        return this;
    }

    Reference* ref();
    Pointer* ptr();
    Array* array_(size_t count = 0);

protected:
    size_t m_size{};
};
} // namespace sdkgenny
