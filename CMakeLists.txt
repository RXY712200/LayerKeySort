cmake_minimum_required(VERSION 3.21)
# CMake accepts a numeric project version; the public header carries preview.1.
project(LayerKeySort VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(LKS_BUILD_TESTS "Build diagnostic tests" ON)
option(LKS_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan where supported" OFF)

set(LKS_SOURCES
    src/lks_alloc.c src/lks_base.c src/path.c src/path_text.c
    src/path_compare.c src/gap.c src/tree.c src/group.c
    src/sort.c src/bulk.c)

function(lks_configure_target target)
    target_compile_features(${target} PUBLIC c_std_17)
    target_include_directories(${target} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    if(LKS_ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PUBLIC -fsanitize=address,undefined)
    endif()
endfunction()

add_library(layerkeysort STATIC ${LKS_SOURCES})
lks_configure_target(layerkeysort)

add_executable(layerkeysort_example examples/basic.c)
target_link_libraries(layerkeysort_example PRIVATE layerkeysort)

if(LKS_BUILD_TESTS)
    include(CTest)
    # Instrumentation is test-only. Production allocations do not touch shared
    # diagnostic counters or fault-injection state.
    add_library(layerkeysort_diagnostics STATIC ${LKS_SOURCES})
    target_compile_definitions(layerkeysort_diagnostics PRIVATE LKS_ENABLE_ALLOC_DIAGNOSTICS)
    lks_configure_target(layerkeysort_diagnostics)
    add_executable(layerkeysort_tests tests/v2_main.c tests/v2_preview.c
        tests/property.c tests/public_api_usage.c)
    target_compile_definitions(layerkeysort_tests PRIVATE LKS_V2_PREVIEW)
    target_include_directories(layerkeysort_tests PRIVATE src tests)
    target_link_libraries(layerkeysort_tests PRIVATE layerkeysort_diagnostics)
    add_test(NAME layerkeysort_v2_preview COMMAND layerkeysort_tests)
endif()
