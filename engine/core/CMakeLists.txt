project(violet-core)

add_library(${PROJECT_NAME} STATIC
    ./private/engine_context.cpp
    ./private/engine_system.cpp
    ./private/engine.cpp
    ./private/plugin.cpp)
add_library(violet::core ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./public
    PRIVATE
        ./private)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        violet::ecs
        violet::task)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)