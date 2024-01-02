#include <numeric>

#include <sdkgenny/namespace.hpp>
#include <sdkgenny/struct.hpp>
#include <sdkgenny/type.hpp>
#include <sdkgenny/variable.hpp>

#include <sdkgenny/template.hpp>

namespace sdkgenny {
Template::VariableCommand* Template::variable(std::string name) {
    return &std::get<VariableCommand>(
        *m_commands.emplace_back(std::make_unique<Command>(VariableCommand{this, std::move(name)})));
}

Struct* Template::instance(const std::map<std::string, Type*>& types) {
    auto instance_name = name() + "<" +
                         std::accumulate(types.begin(), types.end(), std::string{},
                             [](std::string acc, const auto& pair) {
                                 if (!acc.empty()) {
                                     acc += ", ";
                                 }
                                 acc += pair.second->name();
                                 return acc;
                             }) +
                         ">";

    auto* ns = owner<Namespace>();

    if (auto* instance = ns->find<Struct>(instance_name)) {
        return dynamic_cast<Struct*>(instance);
    }

    auto* instance = ns->struct_(instance_name);
    Variable* v = nullptr;

    for (const auto& command : m_commands) {
        switch (command->index()) {
        case VariableCmd:
            v = instance->variable(std::get<VariableCommand>(*command).name);
            break;

        case TypeCmd:
            v->type(std::get<TypeCommand>(*command).type);
            break;

        case OffsetCmd:
            v->offset(std::get<OffsetCommand>(*command).offset);
            break;

        case ParamTypeCmd: {
            auto& name = std::get<ParamTypeCommand>(*command).name;
            auto* type = types.at(name);
            v->type(type);
            break;
        }
        }
    }

    return instance;
}
} // namespace sdkgenny