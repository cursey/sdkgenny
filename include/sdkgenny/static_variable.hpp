#pragma once 

#include <sdkgenny/variable.hpp>

namespace sdkgenny {
	class StaticVariable : public Variable {
	public:
		explicit StaticVariable(std::string_view name);

		void generate(std::ostream& os) const override;

		auto initializer() const { return m_initializer; }
        auto initializer(std::string_view init) { 
			m_initializer = std::string(init);
            return this;
		}

		std::string m_initializer{}; 
	};
}