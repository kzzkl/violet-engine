project(violet-physics)

option(VIOLET_PHYSICS_DEBUG_DRAW "Whether to show rigid body wireframe" OFF)

set(SRC_FILES
    ./private/components/rigidbody_component.cpp
    ./private/physics_context.cpp
    ./private/physics_plugin.cpp
    ./private/physics_scene.cpp
    ./private/physics_system.cpp)

if (VIOLET_PHYSICS_DEBUG_DRAW)
    list(APPEND SRC_FILES ./private/physics_debug.cpp)
endif()

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
add_library(violet::physics ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./interface
    ./public
    PRIVATE
    ./private)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    violet::task
    violet::math
    violet::scene)

if(${VIOLET_PHYSICS_DEBUG_DRAW})
    target_compile_definitions(${PROJECT_NAME} PRIVATE VIOLET_PHYSICS_DEBUG_DRAW)
    target_link_libraries(${PROJECT_NAME} PUBLIC violet::graphics)
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)