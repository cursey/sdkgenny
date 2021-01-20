cmake_minimum_required(VERSION 2.8)

project(SdkGenny)

add_executable(example_car "examples/Car.cpp")
target_compile_features(example_car PRIVATE cxx_std_17)
target_include_directories(example_car PRIVATE "include/")

add_executable(example_usage "examples/Usage.cpp")
target_compile_features(example_usage PRIVATE cxx_std_17)
target_include_directories(example_usage PRIVATE "include/")

