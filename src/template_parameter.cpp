#include <sdkgenny/template_parameter.hpp>

namespace sdkgenny {
TemplateParameter::TemplateParameter(std::string_view name) : Type{name} {
    // Template parameters have no concrete size — they are placeholders.
    // Mark skip_generation so they don't appear in output.
    skip_generation(true);
}
} // namespace sdkgenny
