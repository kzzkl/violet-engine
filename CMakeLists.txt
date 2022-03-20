cmake_minimum_required(VERSION 3.12)

project(ash-engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

add_definitions(-DUNICODE -D_UNICODE)

if (MSVC)
    add_compile_options(/wd4251)
    add_compile_options(/wd4275)
endif()

set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/thirdparty/install)

set(ASH_CORE_INTERFACE_DIR ${PROJECT_SOURCE_DIR}/engine/core/interface)
set(ASH_GRAPHICS_INTERFACE_DIR ${PROJECT_SOURCE_DIR}/engine/graphics/interface)

option(ASH_BUILD_THIRDPARTY "Build thirdparty" OFF)

option(ASH_BUILD_ENGINE "Build engine" ON)
option(ASH_BUILD_TESTING "Build test project" ON)
option(ASH_BUILD_EXAMPLES "Build examples" ON)

message("Build start")

if (ASH_BUILD_THIRDPARTY)
    add_subdirectory(thirdparty)
endif()

if (ASH_BUILD_ENGINE)
    add_subdirectory(engine)
endif()

if (ASH_BUILD_TESTING)
    add_subdirectory(tests)
endif()

if (ASH_BUILD_EXAMPLES)
    add_subdirectory(samples)
endif()