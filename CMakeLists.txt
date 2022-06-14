cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

project(ash-engine)

if (MSVC)
    add_compile_options(/wd4251)
    add_compile_options(/wd4275)
endif()

if (WIN32)
    add_definitions(-DUNICODE)
    add_definitions(-D_UNICODE)
endif()

set(ASH_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ASH_THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/thirdparty)

set(ASH_CORE_INTERFACE_DIR ${PROJECT_SOURCE_DIR}/engine/core/interface)
set(ASH_GRAPHICS_INTERFACE_DIR ${PROJECT_SOURCE_DIR}/engine/graphics/interface)
set(ASH_PHYSICS_INTERFACE_DIR ${PROJECT_SOURCE_DIR}/engine/physics/interface)

option(ASH_BUILD_THIRDPARTY "Build thirdparty" ON)

option(ASH_BUILD_ENGINE "Build engine" ON)
option(ASH_BUILD_TESTING "Build test project" ON)
option(ASH_BUILD_EXAMPLES "Build examples" ON)
option(ASH_BUILD_EDITOR "Build editor" ON)

message("Build start")

if (ASH_BUILD_THIRDPARTY)
    add_subdirectory(thirdparty)
endif()

if (ASH_BUILD_ENGINE)
    add_subdirectory(engine)
endif()

if (ASH_BUILD_EDITOR)
    add_subdirectory(editor)
endif()

if (ASH_BUILD_TESTING)
    add_subdirectory(tests)
endif()

if (ASH_BUILD_EXAMPLES)
    add_subdirectory(samples)
endif()