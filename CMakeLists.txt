cmake_minimum_required(VERSION 3.5)

project(cpp-consistent-hashing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")

option(WITH_PCG32 "Use PCG32 random number generator" OFF)
option(WITH_HEAPSTATS "Enable heap allocation statistics" ON)

find_package(Boost REQUIRED)
find_package(xxHash REQUIRED)
find_package(fmt REQUIRED)
find_package(cxxopts REQUIRED)
find_package(yaml-cpp REQUIRED)
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
    monotonicity-v2.h
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
 "speed_test-v2.h" "unit_tests/YamlParserTest.cc")

if(WITH_PCG32)
    target_include_directories(cpp-consistent-hashing PRIVATE ${PCG_INCLUDE_DIRS})
endif()

target_include_directories(cpp-consistent-hashing PRIVATE ${GTL_INCLUDE_DIRS})

target_link_libraries(cpp-consistent-hashing PRIVATE xxHash::xxhash fmt::fmt cxxopts::cxxopts ${YAML_CPP_LIBRARIES})

include(GNUInstallDirs)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

install(TARGETS cpp-consistent-hashing
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

enable_testing()

add_executable(
  YamlParserTest
  unit_tests/YamlParserTest.cc
)

target_link_libraries(
  YamlParserTest
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(YamlParserTest)