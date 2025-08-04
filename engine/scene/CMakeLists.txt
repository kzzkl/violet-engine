add_library(violet-scene STATIC
    private/hierarchy_system.cpp
    private/scene_system.cpp
    private/transform_system.cpp)
add_library(violet::scene ALIAS violet-scene)

target_include_directories(violet-scene
    PUBLIC
        public)

target_link_libraries(violet-scene
    PUBLIC
        violet::core
        violet::math
        violet::ecs-command)

install(TARGETS violet-scene
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)