#pragma once

#include <sdkgenny/reference.hpp>

namespace sdkgenny {
class Pointer : public Reference {
public:
    explicit Pointer(std::string_view name);

    void generate_typename_for(std::ostream& os, const Object* obj) const override;
};

} // namespace sdkgenny