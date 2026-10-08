cmake_minimum_required(VERSION 3.16)
project(LayerKeySortMini VERSION 1.0.0 LANGUAGES C)
# Release identity: v1.0.0-preview.1 (CMake VERSION is numeric only).
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

function(mini_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
endfunction()

add_library(layerkeysort_mini STATIC src/lks_mini.c)
target_include_directories(layerkeysort_mini PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
mini_warnings(layerkeysort_mini)

add_executable(lks_mini_basic examples/basic.c)
target_link_libraries(lks_mini_basic PRIVATE layerkeysort_mini)
mini_warnings(lks_mini_basic)

include(CTest)
if(BUILD_TESTING)
    add_executable(lks_mini_public_tests tests/test_public.c)
    target_link_libraries(lks_mini_public_tests PRIVATE layerkeysort_mini)
    mini_warnings(lks_mini_public_tests)
    add_test(NAME mini_public COMMAND lks_mini_public_tests)
    # This separate test includes the source to inspect private invariants and
    # substitute allocation locally. It does not link the production library.
    add_executable(lks_mini_internal_tests tests/test_internal.c)
    target_include_directories(lks_mini_internal_tests PRIVATE include)
    mini_warnings(lks_mini_internal_tests)
    add_test(NAME mini_internal COMMAND lks_mini_internal_tests)
    add_test(NAME mini_example COMMAND lks_mini_basic)
endif()
