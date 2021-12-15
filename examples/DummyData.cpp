// This example originally highlighted an issue with include path resolution that
// has since been fixed.
#include "Genny.hpp"

int main() {
    genny::Sdk sdk{};
    auto g = sdk.global_ns();

    sdk.include("REFramework.hpp");
    sdk.include("sdk/ReClass.hpp");
    sdk.include("cstdint");

    g->type("int8_t")->size(1);
    g->type("int16_t")->size(2);
    g->type("int32_t")->size(4);
    g->type("int64_t")->size(8);
    g->type("uint8_t")->size(1);
    g->type("uint16_t")->size(2);
    g->type("uint32_t")->size(4);
    g->type("uint64_t")->size(8);
    g->type("float")->size(4);
    g->type("double")->size(8);
    g->type("bool")->size(1);
    g->type("char")->size(1);
    g->type("int")->size(4);
    g->type("void")->size(0);
    g->type("void*")->size(8);

    auto dummy_type = g->namespace_("sdk")->struct_("DummyData")->size(0x100);

    auto c = g->namespace_("not-sdk")->class_("SomeClass");
    c->function("SomeFunc")->returns(dummy_type);

    sdk.generate("sdk");

    return 1;
}
