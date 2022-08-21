#include "sdkgenny/GenericType.hpp"

namespace sdkgenny {
GenericType::GenericType(std::string_view name) : Type{name} {
    usable_name = [this] {
        std::string name{};
        constexpr auto allowed_chars = "*&[]:<>, ";

        for (auto&& c : m_name) {
            if (!std::isalnum(c) && std::strchr(allowed_chars, c) == nullptr) {
                name += '_';
            } else {
                name += c;
            }
        }

        if (!name.empty() && std::isdigit(name[0])) {
            name = "_" + name;
        }

        return name;
    };
}
} // namespace sdkgenny