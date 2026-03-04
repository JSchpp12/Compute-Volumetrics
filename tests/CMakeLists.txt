# tests/CMakeLists.txt

find_package(GTest CONFIG REQUIRED)

add_executable(unit_tests 
    test_cmdLine.cpp
)

# Mirror useful compile defs/macros you rely on in pure code paths
target_compile_definitions(unit_tests PRIVATE
    "NANOVDB_USE_OPENVDB"
    "GLM_FORCE_DEPTH_ZERO_TO_ONE"
)

# Link minimal deps. Avoid linking windowing or rendering stacks unless necessary.
target_link_libraries(unit_tests PRIVATE
    compute_volumetrics_lib
    GTest::gtest 
    GTest::gtest_main
)

target_compile_features(unit_tests PRIVATE cxx_std_20)

# Auto-discover gtest cases
include(GoogleTest)
gtest_discover_tests(unit_tests)