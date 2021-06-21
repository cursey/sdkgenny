#include "Genny.hpp"

int main(int argc, char* argv[]) {
    genny::Sdk sdk{};
    auto g = sdk.global_ns();

    auto ushort = g->type("unsigned short")->size(2);

    // Generate the Date struct described @ https://docs.microsoft.com/en-us/cpp/cpp/cpp-bit-fields
    auto date = g->struct_("Date");

    date->variable("nWeekDay")->type(ushort)->bit_size(3)->append()->bit_append();
    date->variable("nMonthDay")->type(ushort)->bit_size(6)->append()->bit_append();
    date->variable("nMonth")->type(ushort)->bit_size(5)->append()->bit_append();
    date->variable("nYear")->type(ushort)->bit_size(8)->append()->bit_append();

    sdk.generate(std::filesystem::current_path() / "bitfield_sdk");

    return 0;
}