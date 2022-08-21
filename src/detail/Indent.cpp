#include <ostream>

#include "sdkgenny/detail/Indent.hpp"

namespace sdkgenny::detail {
Indent::Indent(std::streambuf* dest, int indent) : m_dest{dest}, m_indent(indent, ' ') {
}

Indent::Indent(std::ostream& dest, int indent) : m_dest{dest.rdbuf()}, m_indent(indent, ' '), m_owner{&dest} {
    m_owner->rdbuf(this);
}

Indent::~Indent() {
    if (m_owner != nullptr) {
        m_owner->rdbuf(m_dest);
    }
}

int Indent::overflow(int ch) {
    if (m_is_at_start_of_line && ch != '\n') {
        m_dest->sputn(m_indent.data(), m_indent.size());
    }
    m_is_at_start_of_line = ch == '\n';
    return m_dest->sputc(ch);
}
} // namespace sdkgenny::detail