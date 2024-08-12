cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

project(violet-engine)

option(VIOLET_USE_SIMD "Use SIMD" ON)

option(VIOLET_BUILD_THIRDPARTY "Build thirdparty" ON)
option(VIOLET_BUILD_ENGINE "Build engine" ON)
option(VIOLET_BUILD_TESTING "Build test project" ON)
option(VIOLET_BUILD_EXAMPLES "Build examples" ON)
option(VIOLET_BUILD_EDITOR "Build editor" ON)

if(WIN32)
    add_definitions(-DUNICODE)
    add_definitions(-D_UNICODE)
endif()

set(VIOLET_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(VIOLET_THIRDPARTY_DIR ${VIOLET_ROOT_DIR}/thirdparty)
set(VIOLET_SHADER_INCLUDE_DIR ${VIOLET_ROOT_DIR}/engine/assets/shaders/include)

set(VIOLET_BINARY_ROOT_DIR ${CMAKE_INSTALL_PREFIX}/bin)

if(VIOLET_USE_SIMD)
    add_definitions(-DVIOLET_USE_SIMD)
endif()

if(VIOLET_BUILD_THIRDPARTY)
    add_subdirectory(thirdparty)
endif()

if(VIOLET_BUILD_ENGINE)
    add_subdirectory(engine)
endif()

if(VIOLET_BUILD_EDITOR)
    # add_subdirectory(editor)
endif()

if(VIOLET_BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(VIOLET_BUILD_EXAMPLES)
    add_subdirectory(samples)
endif()