#include <cassert>
#include <iostream>

#include <Genny.hpp>

constexpr auto PREAMBLE =
    R"(MIT License

Copyright (c) 2021 cursey

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.)";

int main(int argc, char* argv[]) {
    auto g = std::make_unique<genny::HeaderFile>("Car.hpp");

    g->preamble(PREAMBLE)->postamble("End of Car.hpp");
    g->include("cstdint")->include_local("Types.hpp");

    g->type("bool")->size(1);
    g->type("char")->size(1);
    g->type("short")->size(2);
    g->type("int")->size(4);
    g->type("long")->size(4);
    g->type("long long")->size(8);
    g->type("float")->size(4);
    g->type("double")->size(8);

    g->enum_("CarTypes")->value("TWO_DOOR", 0)->value("FOUR_DOOR", 1)->value("TRUCK", 2)->value("VAN", 3);

    auto vec3 = g->struct_("Vec3")->size(16);

    vec3->member("x")->type(g->type("float"))->offset(0);
    vec3->member("y")->type(g->type("float"))->offset(4);
    vec3->member("z")->type(g->type("float"))->offset(8);

    auto car = g->class_("Car")->parent(g->class_("ModeOfTransportation"));

    car->member("weight")->type(g->type("int"))->offset(8);
    car->member("value")->type(g->type("float"))->offset(12);

    auto wheel = car->class_("Wheel");
    auto color = car->enum_("Color")->value("RED", 0)->value("BLACK", 1);
    auto door = car->struct_("Door");

    wheel->member("size")->type(g->type("int"))->offset(0);
    door->member("color")->type(color)->offset(0);
    car->array_("wheels")->count(4)->type(wheel)->offset(16);
    car->array_("doors")->count(4)->type(door)->offset(16 + 4 * 4);
    car->member("pos")->type(g->type("Vec3"))->offset(car->array_("doors")->end());
    car->member("pos_history")->type(g->type("Vec3")->ptr())->offset(car->member("pos")->end());

    auto drive = car->method("drive");

    drive->returns(g->type("float"));
    drive->param("speed")->type(g->type("float"));
    drive->param("distance")->type(g->type("float"));
    drive->procedure(R"(std::cout << "Oh my god guys I'm driving!"; return 5.0f;)");

    auto open_door = car->virtual_method("open_door");

    open_door->param("where")->type(g->type("Vec3")->ptr());

    car->enum_class("Title")->value("SALVAGE", 0)->value("CLEAN", 1)->type(g->type("long long"));

    auto two_door = g->class_("TwoDoorCar")->parent(g->class_("Car"));

    // Name collision will occur. Will have a number appended in the output.
    two_door->member("weight")->type(g->type("long"))->offset(120);
    assert(two_door->owner<genny::Namespace>() == g.get());

    g->generate(std::cout);

    return 0;
}
