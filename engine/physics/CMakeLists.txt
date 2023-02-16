project(violet-physics)

option(VIOLET_PHYSICS_DEBUG_DRAW "Whether to show rigid body wireframe" OFF)

add_library(${PROJECT_NAME} STATIC
    ./private/joint.cpp
    ./private/physics_plugin.cpp
    ./private/physics.cpp
    ./private/rigidbody.cpp)
add_library(violet::physics ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./interface
    ./public
    PRIVATE
    ./private)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    violet::core
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