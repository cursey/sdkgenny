#pragma once

#include "Struct.hpp"

namespace sdkgenny {
class Class : public Struct {
public:
    explicit Class(std::string_view name);

    void generate_forward_decl(std::ostream& os) const override;
    void generate(std::ostream& os) const override;
};
} // namespace sdkgenny