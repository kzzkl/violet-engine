project(violet-physics)

option(VIOLET_PHYSICS_DEBUG_DRAW "Whether to show rigid body wireframe" OFF)

add_library(${PROJECT_NAME} STATIC
    ./private/components/rigidbody.cpp
    ./private/physics_context.cpp
    ./private/physics_plugin.cpp
    ./private/physics_system.cpp
    ./private/physics_world.cpp)
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