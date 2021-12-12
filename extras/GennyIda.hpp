// SdkGenny - Genny.hpp - A single file header framework for generating C++ compatible SDKs
// https://github.com/cursey/sdkgenny
// GennyIda.hpp is an optional extra for SdkGenny that generates output intended to be consumed by IDA.

#pragma once

#include "Genny.hpp"

namespace genny::ida {
// Does a destructive transformation to the Sdk to make it's output parsable by IDA.
// FIXME: IDA doesn't support enum classes.
inline void transform(Sdk& sdk) {
    auto g = sdk.global_ns();
    std::unordered_set<Type*> types{};
    std::unordered_set<Namespace*> namespaces{};

    g->get_all_in_children<Type>(types);
    g->get_all_in_children<Namespace>(namespaces);

    for (auto&& t : types) {
        if (!t->is_a<Struct>() && !t->is_a<Enum>()) {
            continue;
        }

        auto owners = t->owners<Namespace>();
        std::string new_name = t->name();

        for (auto&& owner : owners) {
            if (!owner->name().empty()) {
                new_name = owner->name() + "::" + new_name;
            }
        }

        t->usable_name = [t, new_name] { return new_name; };
        t->simple_typename_generation(true);
        t->remove_all<Function>();
    }

    sdk.generate_namespaces(false);
}

} // namespace genny::ida
