#pragma once

#include "Reference.hpp"

namespace sdkgenny {
class Pointer : public Reference {
public:
    explicit Pointer(std::string_view name);

    Pointer* ptr();

    void generate_typename_for(std::ostream& os, const Object* obj) const override;
};

} // namespace sdkgenny