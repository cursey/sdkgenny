#include <iostream>
#include <filesystem>

#include "Genny.hpp"

void car(genny::Namespace* g) {
    g->type("bool")->size(1);
    g->type("char")->size(1);
    g->type("short")->size(2);
    g->type("int")->size(4);
    g->type("long")->size(4);
    g->type("long long")->size(8);
    g->type("float")->size(4);
    g->type("double")->size(8);

    g->enum_("CarTypes")->value("TWO_DOOR", 0)->value("FOUR_DOOR", 1)->value("TRUCK", 2)->value("VAN", 3);

    auto vec3 = g->owner<genny::Namespace>()->struct_("Vec3")->size(16);

    vec3->variable("x")->type(g->type("float"))->offset(0);
    vec3->variable("y")->type(g->type("float"))->offset(4);
    vec3->variable("z")->type(g->type("float"))->offset(8);

    auto car = g->class_("Car")->parent(g->class_("ModeOfTransportation"));

    car->variable("weight")->type(g->type("int"))->offset(8);
    car->variable("value")->type(g->type("float"))->offset(12);

    auto wheel = car->class_("Wheel");
    auto color = car->enum_("Color")->value("RED", 0)->value("BLACK", 1);
    auto door = car->struct_("Door");

    wheel->variable("size")->type(g->type("int"))->offset(0);
    door->variable("color")->type(color)->offset(0);
    car->array_("wheels")->count(4)->type(wheel)->offset(16);
    car->array_("doors")->count(4)->type(door)->offset(16 + 4 * 4);
    car->variable("pos")->type(g->type("Vec3"))->offset(car->array_("doors")->end());
    car->variable("pos_history")->type(g->type("Vec3")->ptr())->offset(car->variable("pos")->end());

    auto drive = car->function("drive");

    drive->returns(g->type("float"));
    drive->param("speed")->type(g->type("float"));
    drive->param("distance")->type(g->type("float"));
    drive->procedure(R"(std::cout << "Oh my god guys I'm driving!"; return 5.0f;)");

    auto open_door = car->virtual_function("open_door");

    open_door->param("where")->type(g->type("Vec3")->ptr());

    car->enum_class("Title")->value("SALVAGE", 0)->value("CLEAN", 1)->type(g->type("long long"));

    auto two_door = g->class_("TwoDoorCar")->parent(g->class_("Car"));

    // Name collision will occur. Will have a number appended in the output.
    two_door->variable("weight")->type(g->type("long"))->offset(120);
}

void usage(genny::Namespace* sdk) {
    // Add some basic types to the global namespace.
    sdk->type("int")->size(4);
    sdk->type("float")->size(4);

    // Make an actual namespace.
    auto ns = sdk->namespace_("foobar");

    // Make a class in the namespace.
    auto foo = ns->class_("Foo");

    // Add some members.
    foo->variable("a")->type("int")->offset(0);
    foo->variable("b")->type("float")->offset(4);

    // Make a subclass.
    auto bar = ns->class_("Bar")->parent(foo);

    // Add a member after 'b'.
    bar->variable("c")->type("int")->offset(foo->variable("b")->end());
}

int main(int argc, char* argv[]) {
    genny::Sdk sdk{};
    auto g = sdk.global_ns();

    sdk.include("cstdint")->include("vector");

    g->type("bool")->size(1);
    g->type("char")->size(1);
    g->type("short")->size(2);
    g->type("int")->size(4);
    g->type("long")->size(4);
    g->type("long long")->size(8);
    g->type("float")->size(4);
    g->type("double")->size(8);

    auto SdkEnum = g->enum_("SdkEnum")->value("A", 1)->value("B", 2)->value("C", 3);
    auto a = g->namespace_("a");
    auto AEnum = a->enum_("AEnum")->value("A", 1)->value("B", 2)->value("C", 3);
    auto b = g->namespace_("b");
    auto ba = b->namespace_("ba"); 
    auto BAClass = ba->class_("BAClass");
    BAClass->variable("a_enum")->offset(8)->type(AEnum->ptr());
    auto c = g->namespace_("c");
    auto CClass = c->class_("CClass");
    CClass->variable("ba_class")->type(BAClass->ptr());
    CClass->variable("a_enum")->offset(CClass->variable("ba_class")->end())->type(AEnum);
    CClass->variable("ba_class_2")->offset(CClass->variable("a_enum")->end())->type(BAClass->ptr()->ptr()->ptr());

    car(g->namespace_("car"));
    usage(g->namespace_("usage"));

    auto say_hi = CClass->static_function("say_hi");
    say_hi->returns(g->type("int"));
    say_hi->procedure("std::cout << \"hi\\n\"; return 1;");

    auto drive = BAClass->virtual_function("car_at_pos")->vtable_index(5);
    drive->returns(g->namespace_("car")->class_("Car")->ptr());
    drive->param("pos")->type(g->struct_("Vec3")->ref());

    auto cclasses = g->generic_type("std::vector<c::CClass*>");
    cclasses->template_type(CClass->ptr());
    cclasses->size(sizeof(std::vector<void*>));

    drive->param("cclasses")->type(cclasses->ptr());

    auto sdk_path = std::filesystem::current_path() / "example_sdk";
    std::filesystem::remove_all(sdk_path);
    sdk.generate(sdk_path);

    return 0;
}