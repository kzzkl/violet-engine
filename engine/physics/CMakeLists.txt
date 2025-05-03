project(violet-physics)

option(VIOLET_PHYSICS_DEBUG_DRAW "Whether to show rigid body wireframe" OFF)

set(SRC_FILES
    ./private/physics_context.cpp
    ./private/physics_plugin.cpp
    ./private/physics_scene.cpp
    ./private/physics_system.cpp)

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
    violet::algorithm
    violet::math
    violet::scene
    violet::task)

if(${VIOLET_PHYSICS_DEBUG_DRAW})
    target_compile_definitions(${PROJECT_NAME} PRIVATE VIOLET_PHYSICS_DEBUG_DRAW)
    target_link_libraries(${PROJECT_NAME} PUBLIC violet::graphics)
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)