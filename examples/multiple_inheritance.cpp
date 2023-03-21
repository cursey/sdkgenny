#include <sdkgenny.hpp>

int main(int argc, char* argv[]) {
    sdkgenny::Sdk sdk{};

    auto g = sdk.global_ns();
    g->type("int")->size(4);
    g->type("float")->size(4);

    auto person = g->class_("Person");
    person->variable("age")->type("int")->offset(0);

    auto student = g->class_("Student")->parent(person);
    student->variable("gpa")->type("float")->offset(person->size());

    auto faculty = g->class_("Faculty")->parent(person);
    faculty->variable("wage")->type("int")->offset(person->size());

    auto ta = g->class_("TA")->parent(student)->parent(faculty);
    ta->variable("hours")->type("int")->offset(student->size() + faculty->size());

    sdk.generate(std::filesystem::current_path() / "multiple_inheritance_sdk");

    return 0;
}