#include <iostream>

#include "Genny.hpp"

int main(int argc, char* argv[]) {
    auto g = std::make_unique<genny::Namespace>("");

    g->type("unsigned short")->size(2);

    // Generate the Date struct described @ https://docs.microsoft.com/en-us/cpp/cpp/cpp-bit-fields
    auto date = g->struct_("Date");
    auto bf = date->bitfield(0);
    
    bf->type("unsigned short");
    bf->field("nWeekDay")->offset(0)->size(3);
    bf->field("nMonthDay")->offset(3)->size(6);
    bf->field("nMonth")->offset(bf->field("nMonthDay")->end())->size(5);
    bf->field("nYear")->offset(16)->size(8);

    g->generate(std::cout);

    return 0;
}