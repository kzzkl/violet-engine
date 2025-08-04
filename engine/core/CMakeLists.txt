add_library(violet-core STATIC
    private/engine_context.cpp
    private/engine.cpp
    private/plugin.cpp)
add_library(violet::core ALIAS violet-core)

target_include_directories(violet-core
    PUBLIC
        public
    PRIVATE
        private)

target_link_libraries(violet-core
    PUBLIC
        violet::ecs
        violet::task)

install(TARGETS violet-core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)