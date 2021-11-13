cmake_minimum_required(VERSION 3.16)

# Check if CMAKE_TOOLCHAIN_FILE is set and use it if it is not
if(NOT CMAKE_TOOLCHAIN_FILE)
    message("CMAKE_TOOLCHAIN_FILE not detected, setting up vcpkg...")

    # Download and setup vcpkg in CMake. Download master tarball from https://github.com/microsoft/vcpkg/archive
    if (NOT IS_DIRECTORY "${CMAKE_BINARY_DIR}/vcpkg-master")
        message("Downloading vcpkg...")

        file(DOWNLOAD "https://github.com/microsoft/vcpkg/archive/master.tar.gz" "${CMAKE_BINARY_DIR}/master.tar.gz")
        file(ARCHIVE_EXTRACT INPUT "${CMAKE_BINARY_DIR}/master.tar.gz" DESTINATION "${CMAKE_BINARY_DIR}/")
    endif()

    # Set CMAKE_TOOLCHAIN_FILE to point to vcpkg/scripts/buildsystems/vcpkg.cmake
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_BINARY_DIR}/vcpkg-master/scripts/buildsystems/vcpkg.cmake")
endif()

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

add_executable(example_badname "examples/BadName.cpp")
target_compile_features(example_badname PRIVATE cxx_std_17)
target_include_directories(example_badname PRIVATE "include/")

option(BUILD_PARSER_EXAMPLE "Build the example that uses GennyParser.hpp")

if(BUILD_PARSER_EXAMPLE)
    find_package(pegtl CONFIG REQUIRED)
    add_executable(example_parser "examples/Parser.cpp" "include/Genny.hpp")
    target_compile_features(example_parser PRIVATE cxx_std_17)
    target_include_directories(example_parser PRIVATE "include/")
    target_link_libraries(example_parser PRIVATE taocpp::pegtl)
    target_compile_definitions(example_parser PRIVATE GENNY_INCLUDE_PARSER)
endif()
