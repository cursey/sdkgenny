#pragma once

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string_view>

#include "Object.hpp"
#include "Type.hpp"

namespace sdkgenny {
class Variable : public Object {
public:
    explicit Variable(std::string_view name) : Object{name} {}

    auto type() const { return m_type; }
    auto type(Type* type) {
        m_type = type;
        return this;
    }

    // Helper that recurses though owners to find the correct type.
    auto type(std::string_view name) {
        m_type = find_in_owners_or_add<Type>(name);
        return this;
    }

    auto offset() const { return m_offset; }
    auto offset(uintptr_t offset) {
        m_offset = offset;
        return this;
    }

    // Sets the offset to be after the last variable in the struct.
    Variable* append();

    virtual size_t size() const {
        if (m_type == nullptr) {
            return 0;
        }

        return m_type->size();
    }

    auto end() const { return offset() + size(); }

    auto bit_size(size_t size) {
        // assert(size <= m_type->size() * CHAR_BIT);
        m_bit_size = size;
        return this;
    }
    auto bit_size() const { return m_bit_size; }

    auto bit_offset(uintptr_t offset) {
        // assert(offset < m_type->size() * CHAR_BIT);
        m_bit_offset = offset;
        return this;
    }
    auto bit_offset() const { return m_bit_offset; }

    auto is_bitfield() const { return m_bit_size != 0; }

    // Call this after append() or offset()
    Variable* bit_append();

    virtual void generate(std::ostream& os) const {
        generate_metadata(os);
        m_type->generate_typename_for(os, this);
        os << " " << usable_name();
        m_type->generate_variable_postamble(os);

        if (m_bit_size != 0) {
            os << " : " << std::dec << m_bit_size;
        }

        os << "; // 0x" << std::hex << m_offset << "\n";
    }

protected:
    Type* m_type{};
    uintptr_t m_offset{};
    size_t m_bit_size{};
    uintptr_t m_bit_offset{};
};
} // namespace sdkgenny