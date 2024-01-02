#pragma once

#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <sdkgenny/object.hpp>

namespace sdkgenny {
class Type;
class Variable;
class Struct;

class Template : public Object {
public:
    struct VariableCommand;

    struct TypeCommand {
        VariableCommand* owner{};
        Type* type{};
    };

    struct OffsetCommand {
        VariableCommand* owner{};
        uintptr_t offset{};
    };

    struct ParamTypeCommand {
        VariableCommand* owner{};
        std::string name{};
    };

    struct VariableCommand {
        Template* owner{};
        std::string name{};

        auto* type(Type* type) {
            owner->m_commands.emplace_back(std::make_unique<Command>(TypeCommand{this, type}));
            return this;
        }

        auto* template_param(std::string param_name) {
            owner->m_commands.emplace_back(std::make_unique<Command>(ParamTypeCommand{this, std::move(param_name)}));
            return this;
        }

        auto* offset(uintptr_t offset) {
            owner->m_commands.emplace_back(std::make_unique<Command>(OffsetCommand{this, offset}));
            return this;
        }
    };

    explicit Template(std::string_view name) : Object{name} {}

    VariableCommand* variable(std::string name);
    Struct* instance(const std::map<std::string, Type*>& types);

private:
    enum CommandType { TypeCmd, OffsetCmd, VariableCmd, ParamTypeCmd };
    using Command = std::variant<TypeCommand, OffsetCommand, VariableCommand, ParamTypeCommand>;

    std::vector<std::unique_ptr<Command>> m_commands{};
};
} // namespace sdkgenny