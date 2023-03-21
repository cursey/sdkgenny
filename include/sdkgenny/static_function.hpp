#pragma once

#include <sdkgenny/function.hpp>

namespace sdkgenny {
class StaticFunction : public Function {
public:
    explicit StaticFunction(std::string_view name);

    void generate(std::ostream& os) const override;
};
} // namespace sdkgenny