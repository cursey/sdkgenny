#pragma once

#include <sdkgenny/type.hpp>

namespace sdkgenny {
class TemplateParameter : public Type {
public:
    explicit TemplateParameter(std::string_view name);
};
} // namespace sdkgenny
