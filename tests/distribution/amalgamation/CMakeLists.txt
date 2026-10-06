cmake_minimum_required(VERSION 3.21)
project(LayerKeySortAmalgamationConsumer LANGUAGES C CXX)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT LKS_AMALGAM_DIR OR NOT EXISTS "${LKS_AMALGAM_DIR}/layerkeysort.c")
    message(FATAL_ERROR "LKS_AMALGAM_DIR must point to a generated package")
endif()
add_executable(amalgamation_consumer
    "${LKS_AMALGAM_DIR}/layerkeysort.c" consumer.c)
target_include_directories(amalgamation_consumer PRIVATE "${LKS_AMALGAM_DIR}")
add_executable(amalgamation_package_example
    "${LKS_AMALGAM_DIR}/layerkeysort.c" "${LKS_AMALGAM_DIR}/example.c")
target_include_directories(amalgamation_package_example PRIVATE "${LKS_AMALGAM_DIR}")
if(MSVC)
    target_compile_options(amalgamation_consumer PRIVATE /W4 /WX)
    target_compile_options(amalgamation_package_example PRIVATE /W4 /WX)
else()
    target_compile_options(amalgamation_consumer PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(amalgamation_package_example PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(amalgamation_library STATIC "${LKS_AMALGAM_DIR}/layerkeysort.c")
add_executable(amalgamation_cpp consumer.cpp)
target_include_directories(amalgamation_cpp PRIVATE "${LKS_AMALGAM_DIR}")
target_link_libraries(amalgamation_cpp PRIVATE amalgamation_library)

add_executable(amalgamation_api_cpp api_compile.cpp)
target_include_directories(amalgamation_api_cpp PRIVATE "${LKS_AMALGAM_DIR}")
target_link_libraries(amalgamation_api_cpp PRIVATE amalgamation_library)
