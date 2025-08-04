option(VIOLET_PHYSICS_DEBUG_DRAW "Whether to show rigid body wireframe" OFF)

add_library(violet-physics STATIC
    private/physics_context.cpp
    private/physics_plugin.cpp
    private/physics_scene.cpp
    private/physics_system.cpp)
add_library(violet::physics ALIAS violet-physics)

target_include_directories(violet-physics
    PUBLIC
        public
    PRIVATE
        private)

target_link_libraries(violet-physics
    PUBLIC
        violet::algorithm
        violet::math
        violet::scene
        violet::task)

if(${VIOLET_PHYSICS_DEBUG_DRAW})
    target_compile_definitions(violet-physics PRIVATE VIOLET_PHYSICS_DEBUG_DRAW)
    target_link_libraries(violet-physics PUBLIC violet::graphics)
endif()

install(TARGETS violet-physics
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)