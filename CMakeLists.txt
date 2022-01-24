# CMakeList.txt : CMake project for porth-cpp, include source and define project
# specific logic here.
#
cmake_minimum_required(VERSION 3.8)

project(
    "porth-cpp"
    LANGUAGES CXX
    VERSION 0.1.0
    DESCRIPTION "Porth in C++"
    HOMEPAGE_URL "https://github.com/Phytolizer/porth-cpp"
)

set(CMAKE_CXX_STANDARD 20)

configure_file("config.hpp.cin" "${PROJECT_BINARY_DIR}/include/config.hpp")

# Add source to this project's executable.
add_library(iota "modules/iota/source/iota.cpp")
target_include_directories(iota PUBLIC "modules/iota/include")

add_executable(iota_driver "modules/iota_driver/source/main.cpp")
target_link_libraries(iota_driver PRIVATE iota)

add_custom_command(
    OUTPUT "${PROJECT_BINARY_DIR}/include/iota_generated/op_id.hpp"
    DEPENDS "${PROJECT_SOURCE_DIR}/modules/porth/iota/op_id.iota"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${PROJECT_BINARY_DIR}/include/iota_generated"
    COMMAND
        "$<TARGET_FILE:iota_driver>"
        "${PROJECT_SOURCE_DIR}/modules/porth/iota/op_id.iota"
        "${PROJECT_BINARY_DIR}/include/iota_generated/op_id.hpp"
)

add_library(subprocess_h INTERFACE)
target_include_directories(
    subprocess_h INTERFACE "modules/subprocess_h/include"
)

add_executable(
    porth_cpp
    "modules/porth/source/main.cpp" "modules/porth/source/lexer.cpp"
    "modules/porth/source/op.cpp"
    "${PROJECT_BINARY_DIR}/include/iota_generated/op_id.hpp"
)
target_link_libraries(porth_cpp PRIVATE subprocess_h)
target_include_directories(
    porth_cpp PRIVATE "modules/porth/include" "${PROJECT_BINARY_DIR}/include"
)

file(
    GENERATE
    OUTPUT "${PROJECT_BINARY_DIR}/include/testconfig.hpp"
    CONTENT "#pragma once\n#define PORTH_CPP_EXE \"$<TARGET_FILE:porth_cpp>\"\n"
)

add_executable(
    porth_test "modules/porth_test/source/main.cpp"
               "${PROJECT_BINARY_DIR}/include/testconfig.hpp"
)
add_dependencies(porth_test porth_cpp)
target_link_libraries(porth_test PRIVATE subprocess_h)
target_include_directories(porth_test PRIVATE "${PROJECT_BINARY_DIR}/include")

if(PROJECT_IS_TOP_LEVEL)
    enable_testing()
    add_test(
        NAME porth_test
        COMMAND "$<TARGET_FILE:porth_test>"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
endif()
