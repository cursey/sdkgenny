#pragma once

#include <sdkgenny/enum.hpp>

namespace sdkgenny {
class EnumClass : public Enum {
public:
    explicit EnumClass(std::string_view name);

    void generate_forward_decl(std::ostream& os) const override;
    void generate(std::ostream& os) const override;
};
} // namespace sdkgenny