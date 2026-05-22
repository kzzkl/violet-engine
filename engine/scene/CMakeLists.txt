add_library(violet-scene STATIC
    private/scene_system.cpp
    private/transform_system.cpp)
add_library(violet::scene ALIAS violet-scene)

target_include_directories(violet-scene
    PUBLIC
        public)

target_link_libraries(violet-scene
    PUBLIC
        violet::core
        violet::math)

install(TARGETS violet-scene
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)