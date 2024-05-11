cmake_minimum_required(VERSION 3.5)

project(cpp-consistent-hashing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

option(WITH_PCG32 "Use PCG32 random number generator" OFF)
option(WITH_HEAPSTATS "Enable heap allocation statistics" ON)

find_package(Boost REQUIRED)
find_package(xxHash REQUIRED)
find_package(fmt REQUIRED)
find_package(cxxopts REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(GTest REQUIRED)
find_path(GTL_INCLUDE_DIRS "gtl/adv_utils.hpp")
include_directories(${YAML_CPP_INCLUDE_DIR})

if(WITH_PCG32)
    find_path(PCG_INCLUDE_DIRS "pcg_extras.hpp")
    add_definitions(-DUSE_PCG32)
endif()

if(WITH_HEAPSTATS)
    add_definitions(-DUSE_HEAPSTATS)
endif()

add_executable(cpp-consistent-hashing main.cpp
    "metrics/monotonicity.h"
    "metrics/balance.h"
    vcpkg.json
    memento/memento.h
    memento/mementoengine.h
    anchor/AnchorHashQre.cpp
    anchor/AnchorHashQre.hpp
    anchor/misc/crc32c_sse42_u64.h
    anchor/anchorengine.h
    memento/mashtable.h
    dx/dxEngine.h
    jump/jumpengine.h
    power/powerengine.h
    utils.h
    utils.cpp
    metrics/resize_time.h
    metrics/lookup_time.h
    YamlParser/YamlParser.h
    metrics/init_time.h "CsvWriter/csv_structures.h" "CsvWriter/csv_writer_handler.h")

enable_testing()
add_executable(yaml-parser-tests unit_tests/test_yaml_parser.cpp
    "metrics/monotonicity.h"
    "metrics/balance.h"
    vcpkg.json
    memento/memento.h
    memento/mementoengine.h
    anchor/AnchorHashQre.cpp
    anchor/AnchorHashQre.hpp
    anchor/misc/crc32c_sse42_u64.h
    anchor/anchorengine.h
    memento/mashtable.h
    dx/dxEngine.h
    jump/jumpengine.h
    power/powerengine.h
    utils.h
    utils.cpp
    metrics/lookup_time.h
    metrics/resize_time.h
    YamlParser/YamlParser.h 
    metrics/init_time.h "CsvWriter/csv_structures.h" "CsvWriter/csv_writer_handler.h")

enable_testing()
add_executable(csv-output-tests unit_tests/test_csv_writer.cpp
    "metrics/monotonicity.h"
    "metrics/balance.h"
    vcpkg.json
    memento/memento.h
    memento/mementoengine.h
    anchor/AnchorHashQre.cpp
    anchor/AnchorHashQre.hpp
    anchor/misc/crc32c_sse42_u64.h
    anchor/anchorengine.h
    memento/mashtable.h
    dx/dxEngine.h
    jump/jumpengine.h
    power/powerengine.h
    utils.h
    utils.cpp
    metrics/lookup_time.h
    metrics/resize_time.h
    YamlParser/YamlParser.h 
    metrics/init_time.h "CsvWriter/csv_structures.h" "CsvWriter/csv_writer_handler.h")

 enable_testing()
add_executable(csv-output-tests2 unit_tests/test_csv_writer_handler.cpp
    "metrics/monotonicity.h"
    "metrics/balance.h"
    vcpkg.json
    memento/memento.h
    memento/mementoengine.h
    anchor/AnchorHashQre.cpp
    anchor/AnchorHashQre.hpp
    anchor/misc/crc32c_sse42_u64.h
    anchor/anchorengine.h
    memento/mashtable.h
    dx/dxEngine.h
    jump/jumpengine.h
    power/powerengine.h
    utils.h
    utils.cpp
    metrics/lookup_time.h
    metrics/resize_time.h
    YamlParser/YamlParser.h 
    metrics/init_time.h "CsvWriter/csv_structures.h" "CsvWriter/csv_writer_handler.h")


if(WITH_PCG32)
    target_include_directories(cpp-consistent-hashing PRIVATE ${PCG_INCLUDE_DIRS})
endif()

target_include_directories(cpp-consistent-hashing PRIVATE ${GTL_INCLUDE_DIRS})

target_link_libraries(cpp-consistent-hashing PRIVATE
    xxHash::xxhash 
    fmt::fmt 
    cxxopts::cxxopts 
    ${YAML_CPP_LIBRARIES}
)

target_link_libraries(yaml-parser-tests PRIVATE
    GTest::gtest_main
    GTest::gtest
    xxHash::xxhash 
    fmt::fmt 
    cxxopts::cxxopts 
    ${YAML_CPP_LIBRARIES}
)
include(GoogleTest)
gtest_discover_tests(yaml-parser-tests)

target_link_libraries(csv-output-tests PRIVATE
    GTest::gtest_main
    GTest::gtest
    xxHash::xxhash 
    fmt::fmt 
    cxxopts::cxxopts 
    ${YAML_CPP_LIBRARIES}
)
include(GoogleTest)
gtest_discover_tests(csv-output-tests)

target_link_libraries(csv-output-tests2 PRIVATE
    GTest::gtest_main
    GTest::gtest
    xxHash::xxhash 
    fmt::fmt 
    cxxopts::cxxopts 
    ${YAML_CPP_LIBRARIES}
)
include(GoogleTest)
gtest_discover_tests(csv-output-tests2)

include(GNUInstallDirs)

install(TARGETS cpp-consistent-hashing
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

