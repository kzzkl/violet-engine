add_library(violet-ecs-command STATIC
    private/ecs_command_system.cpp)
add_library(violet::ecs-command ALIAS violet-ecs-command)

target_include_directories(violet-ecs-command
    PUBLIC
        public
    PRIVATE
        private)

target_link_libraries(violet-ecs-command
    PUBLIC
        violet::core)

install(TARGETS violet-ecs-command
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)