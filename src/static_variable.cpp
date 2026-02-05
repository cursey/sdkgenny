#pragma once 

#include <sdkgenny/static_variable.hpp>

namespace sdkgenny {
	StaticVariable::StaticVariable(std::string_view name) : Variable{name} {
	}

	void StaticVariable::generate(std::ostream& os) const {
            generate_comment(os);
            generate_metadata(os);

            os << "static ";
            m_type->generate_typename_for(os, this);
            os << " " << usable_name();
            m_type->generate_variable_postamble(os);

            if (m_bit_size != 0) {
                os << " : " << std::dec << m_bit_size;
            }

            os << "; // 0x" << std::hex << m_offset << "\n";
    }
}
