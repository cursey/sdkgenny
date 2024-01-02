#pragma once

#include <sdkgenny/typename.hpp>

namespace sdkgenny {
class Class;
class Enum;
class EnumClass;
class GenericType;
class Struct;
class Type;
class Template;

class Namespace : public Typename {
public:
    explicit Namespace(std::string_view name);

    Type* type(std::string_view name);
    GenericType* generic_type(std::string_view name);
    Struct* struct_(std::string_view name);
    Class* class_(std::string_view name);
    Enum* enum_(std::string_view name);
    EnumClass* enum_class(std::string_view name);
    Namespace* namespace_(std::string_view name);
    Template* template_(std::string_view name);
};
} // namespace sdkgenny