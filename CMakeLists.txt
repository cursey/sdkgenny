cmake_minimum_required(VERSION 3.5)

project(SdkGenny)

add_executable(example_car "examples/Car.cpp")
target_compile_features(example_car PRIVATE cxx_std_17)
target_include_directories(example_car PRIVATE "include/")

add_executable(example_bitfield "examples/Bitfield.cpp")
target_compile_features(example_bitfield PRIVATE cxx_std_17)
target_include_directories(example_bitfield PRIVATE "include/")

add_executable(example_typename "examples/Typename.cpp")
target_compile_features(example_typename PRIVATE cxx_std_17)
target_include_directories(example_typename PRIVATE "include/")

add_executable(example_usage "examples/Usage.cpp")
target_compile_features(example_usage PRIVATE cxx_std_17)
target_include_directories(example_usage PRIVATE "include/")

add_executable(example_sdk "examples/Sdk.cpp")
target_compile_features(example_sdk PRIVATE cxx_std_17)
target_include_directories(example_sdk PRIVATE "include/")

add_executable(example_dummydata "examples/DummyData.cpp")
target_compile_features(example_dummydata PRIVATE cxx_std_17)
target_include_directories(example_dummydata PRIVATE "include/")


option(BUILD_PARSER_EXAMPLE "Build the example that uses GennyParser.hpp")

if(BUILD_PARSER_EXAMPLE)
    find_package(pegtl CONFIG REQUIRED)
    add_executable(example_parser "examples/Parser.cpp")
    target_compile_features(example_parser PRIVATE cxx_std_17)
    target_include_directories(example_parser PRIVATE "include/")
    target_link_libraries(example_parser PRIVATE taocpp::pegtl)
endif()
