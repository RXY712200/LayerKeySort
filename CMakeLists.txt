cmake_minimum_required(VERSION 3.21)
# CMake accepts a numeric project version; the public header carries the full version string.
project(LayerKeySort VERSION 3.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(PROJECT_IS_TOP_LEVEL)
    set(LKS_EMBEDDED_DEFAULT ON)
else()
    set(LKS_EMBEDDED_DEFAULT OFF)
endif()
option(LKS_BUILD_TESTS "Build diagnostic tests" ${LKS_EMBEDDED_DEFAULT})
option(LKS_BUILD_BENCHMARKS "Build the public-API benchmark harness" OFF)
option(LKS_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan where supported" OFF)
option(LKS_BUILD_EXAMPLES "Build LayerKeySort examples" ${LKS_EMBEDDED_DEFAULT})
option(LKS_INSTALL "Generate LayerKeySort install rules" ${LKS_EMBEDDED_DEFAULT})
option(LKS_BUILD_RESEARCH "Build branch-only tail-cost research configurations" OFF)

include(GNUInstallDirs)
include(cmake/ProductionSources.cmake)

function(lks_configure_target target)
    target_compile_features(${target} PUBLIC c_std_17)
    target_include_directories(${target} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    if(LKS_ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PUBLIC -fsanitize=address,undefined)
    endif()
endfunction()

add_library(layerkeysort STATIC ${LKS_SOURCES})
add_library(LayerKeySort::layerkeysort ALIAS layerkeysort)
lks_configure_target(layerkeysort)

if(LKS_BUILD_EXAMPLES)
    add_executable(layerkeysort_example examples/basic.c)
    target_link_libraries(layerkeysort_example PRIVATE layerkeysort)
    add_executable(layerkeysort_ordered_example examples/ordered_tree.c)
    target_link_libraries(layerkeysort_ordered_example PRIVATE layerkeysort)
    add_executable(layerkeysort_layer_list_example examples/layer_list.c)
    target_link_libraries(layerkeysort_layer_list_example PRIVATE layerkeysort)
endif()

if(LKS_BUILD_BENCHMARKS)
    add_executable(layerkeysort_benchmark benchmarks/current_benchmark.c)
    target_link_libraries(layerkeysort_benchmark PRIVATE layerkeysort)
    add_executable(layerkeysort_workload benchmarks/workload_simulator.c)
    lks_configure_target(layerkeysort_workload)
    target_include_directories(layerkeysort_workload PRIVATE src)
    target_link_libraries(layerkeysort_workload PRIVATE layerkeysort)
endif()

if(LKS_INSTALL)
    include(CMakePackageConfigHelpers)
    set(LKS_PACKAGE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/LayerKeySort")
    configure_package_config_file(cmake/LayerKeySortConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/LayerKeySortConfig.cmake"
        INSTALL_DESTINATION "${LKS_PACKAGE_DIR}")
    # Same-major source compatibility for locally built static packages;
    # this does not promise cross-toolchain binary ABI compatibility.
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/LayerKeySortConfigVersion.cmake"
        VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMajorVersion)
    install(TARGETS layerkeysort EXPORT LayerKeySortTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(FILES include/layerkeysort.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(FILES LICENSE DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/LayerKeySort")
    install(EXPORT LayerKeySortTargets NAMESPACE LayerKeySort::
        DESTINATION "${LKS_PACKAGE_DIR}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/LayerKeySortConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/LayerKeySortConfigVersion.cmake"
        DESTINATION "${LKS_PACKAGE_DIR}")
endif()

if(LKS_BUILD_TESTS)
    include(CTest)
    # Instrumentation and historical v1 owned-base merge are test-only.
    # Production has neither shared diagnostic state nor v1 merge machinery.
    add_library(layerkeysort_diagnostics STATIC ${LKS_SOURCES})
    target_compile_definitions(layerkeysort_diagnostics PRIVATE
        LKS_ENABLE_ALLOC_DIAGNOSTICS LKS_ENABLE_V1_REGRESSION_HELPERS)
    lks_configure_target(layerkeysort_diagnostics)
    if(LKS_BUILD_RESEARCH)
        target_compile_definitions(layerkeysort_diagnostics PRIVATE LKS_RESEARCH_TRACE)
    endif()
    add_executable(layerkeysort_tests tests/v2_main.c tests/v2_preview.c
        tests/property.c tests/mutation.c tests/path_external.c tests/public_api_usage.c
        tests/v3_ordered.c)
    target_compile_definitions(layerkeysort_tests PRIVATE
        LKS_V2_PREVIEW LKS_ENABLE_V1_REGRESSION_HELPERS)
    target_include_directories(layerkeysort_tests PRIVATE src tests)
    target_link_libraries(layerkeysort_tests PRIVATE layerkeysort_diagnostics)
    add_test(NAME layerkeysort_v3_core COMMAND layerkeysort_tests)
    add_executable(layerkeysort_soak tests/soak.c)
    target_include_directories(layerkeysort_soak PRIVATE src)
    target_link_libraries(layerkeysort_soak PRIVATE layerkeysort_diagnostics)
    add_test(NAME layerkeysort_mutation_soak COMMAND layerkeysort_soak)
    add_executable(layerkeysort_consumer_roundtrip tests/consumer_roundtrip.c)
    target_link_libraries(layerkeysort_consumer_roundtrip PRIVATE layerkeysort)
    add_test(NAME layerkeysort_consumer_roundtrip COMMAND layerkeysort_consumer_roundtrip)
    add_executable(layerkeysort_parser_torture tests/parser_torture.c)
    target_link_libraries(layerkeysort_parser_torture PRIVATE layerkeysort)
    add_test(NAME layerkeysort_parser_torture COMMAND layerkeysort_parser_torture)
    add_executable(layerkeysort_rc_contract tests/rc_contract.c)
    target_include_directories(layerkeysort_rc_contract PRIVATE src)
    target_link_libraries(layerkeysort_rc_contract PRIVATE layerkeysort_diagnostics)
    add_test(NAME layerkeysort_rc_contract COMMAND layerkeysort_rc_contract)
    if(LKS_BUILD_BENCHMARKS)
        add_executable(layerkeysort_workload_diagnostics benchmarks/workload_simulator.c)
        lks_configure_target(layerkeysort_workload_diagnostics)
        target_include_directories(layerkeysort_workload_diagnostics PRIVATE src)
        target_compile_definitions(layerkeysort_workload_diagnostics PRIVATE LKS_WORKLOAD_DIAGNOSTICS)
        if(LKS_BUILD_RESEARCH)
            target_compile_definitions(layerkeysort_workload_diagnostics PRIVATE LKS_RESEARCH_TRACE)
        endif()
        target_link_libraries(layerkeysort_workload_diagnostics PRIVATE layerkeysort_diagnostics)
        foreach(family timeline priority local churn)
            add_test(NAME layerkeysort_workload_${family}_smoke
                COMMAND layerkeysort_workload_diagnostics ${family} 7 128 512 128)
        endforeach()
        find_package(Python3 COMPONENTS Interpreter QUIET)
        if(Python3_Interpreter_FOUND)
            add_test(NAME layerkeysort_workload_output_smoke
                COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/run_workloads.py"
                    --timed $<TARGET_FILE:layerkeysort_workload>
                    --diagnostic $<TARGET_FILE:layerkeysort_workload_diagnostics> --smoke)
        endif()
        add_test(NAME layerkeysort_benchmark_tree_smoke
            COMMAND layerkeysort_benchmark tree equal 200 0 1)
        add_test(NAME layerkeysort_benchmark_mutation_smoke
            COMMAND layerkeysort_benchmark mut_mixed random 200 0 1)
        add_test(NAME layerkeysort_benchmark_sort_smoke
            COMMAND layerkeysort_benchmark sort duplicates 200 0 1)
    endif()
endif()

# Research targets are opt-in and never installed/exported. All configurations
# use one tree.c; the normal library remains strategy zero.
if(LKS_BUILD_RESEARCH)
    if(NOT LKS_BUILD_TESTS OR NOT LKS_BUILD_BENCHMARKS)
        message(FATAL_ERROR "Research requires tests and benchmark targets")
    endif()
    set(research_id 0)
    add_executable(lks_research_baseline_oom benchmarks/research_oom.c)
    target_include_directories(lks_research_baseline_oom PRIVATE src)
    target_link_libraries(lks_research_baseline_oom PRIVATE layerkeysort_diagnostics)
    add_test(NAME research_baseline_oom COMMAND lks_research_baseline_oom)
    foreach(strategy direct7 direct8 depth_bias equal8 open_range open_dense)
        math(EXPR research_id "${research_id} + 1")
        foreach(mode timed diagnostic)
            set(research_lib "lks_research_${strategy}_${mode}")
            add_library(${research_lib} STATIC ${LKS_SOURCES})
            lks_configure_target(${research_lib})
            target_compile_definitions(${research_lib} PRIVATE LKS_RESEARCH_RELABEL_STRATEGY=${research_id})
            if(mode STREQUAL "diagnostic")
                target_compile_definitions(${research_lib} PRIVATE
                    LKS_ENABLE_ALLOC_DIAGNOSTICS LKS_ENABLE_V1_REGRESSION_HELPERS LKS_RESEARCH_TRACE)
            endif()
            set(workload "lks_workload_${strategy}_${mode}")
            add_executable(${workload} benchmarks/workload_simulator.c)
            lks_configure_target(${workload})
            target_include_directories(${workload} PRIVATE src)
            target_compile_definitions(${workload} PRIVATE LKS_RESEARCH_RELABEL_STRATEGY=${research_id})
            if(mode STREQUAL "diagnostic")
                target_compile_definitions(${workload} PRIVATE LKS_WORKLOAD_DIAGNOSTICS LKS_RESEARCH_TRACE)
            endif()
            target_link_libraries(${workload} PRIVATE ${research_lib})
        endforeach()
        set(diag "lks_research_${strategy}_diagnostic")
        set(timed "lks_research_${strategy}_timed")
        add_executable(lks_oom_${strategy} benchmarks/research_oom.c)
        target_include_directories(lks_oom_${strategy} PRIVATE src)
        target_link_libraries(lks_oom_${strategy} PRIVATE ${diag})
        add_test(NAME research_${strategy}_oom COMMAND lks_oom_${strategy})
        add_executable(lks_core_${strategy} tests/v2_main.c tests/v2_preview.c tests/property.c
            tests/mutation.c tests/path_external.c tests/public_api_usage.c tests/v3_ordered.c)
        target_compile_definitions(lks_core_${strategy} PRIVATE LKS_V2_PREVIEW LKS_ENABLE_V1_REGRESSION_HELPERS)
        target_include_directories(lks_core_${strategy} PRIVATE src tests)
        target_link_libraries(lks_core_${strategy} PRIVATE ${diag})
        add_test(NAME research_${strategy}_core COMMAND lks_core_${strategy})
        foreach(case soak consumer_roundtrip parser_torture rc_contract)
            add_executable(lks_${strategy}_${case} tests/${case}.c)
            target_include_directories(lks_${strategy}_${case} PRIVATE src)
            if(case STREQUAL "soak" OR case STREQUAL "rc_contract")
                target_link_libraries(lks_${strategy}_${case} PRIVATE ${diag})
            else()
                target_link_libraries(lks_${strategy}_${case} PRIVATE ${timed})
            endif()
            add_test(NAME research_${strategy}_${case} COMMAND lks_${strategy}_${case})
        endforeach()
        foreach(family timeline priority local churn)
            add_test(NAME research_${strategy}_${family}_smoke
                COMMAND lks_workload_${strategy}_diagnostic ${family} 7 128 512 128)
        endforeach()
        if(Python3_Interpreter_FOUND)
            add_test(NAME research_${strategy}_output_smoke COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/run_workloads.py"
                --timed $<TARGET_FILE:lks_workload_${strategy}_timed>
                --diagnostic $<TARGET_FILE:lks_workload_${strategy}_diagnostic> --smoke)
        endif()
        add_executable(lks_benchmark_${strategy} benchmarks/current_benchmark.c)
        target_link_libraries(lks_benchmark_${strategy} PRIVATE ${timed})
        add_test(NAME research_${strategy}_benchmark_tree COMMAND lks_benchmark_${strategy} tree equal 200 0 1)
        add_test(NAME research_${strategy}_benchmark_mutation COMMAND lks_benchmark_${strategy} mut_mixed random 200 0 1)
        add_test(NAME research_${strategy}_benchmark_sort COMMAND lks_benchmark_${strategy} sort duplicates 200 0 1)
    endforeach()
endif()
