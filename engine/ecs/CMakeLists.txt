add_library(violet-ecs STATIC
    private/archetype_chunk.cpp
    private/archetype.cpp
    private/view.cpp
    private/world_command.cpp
    private/world.cpp)
add_library(violet::ecs ALIAS violet-ecs)

target_include_directories(violet-ecs
    PUBLIC
        public
    PRIVATE
        private)

target_link_libraries(violet-ecs
    PUBLIC
        violet::common)

install(TARGETS violet-ecs
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)