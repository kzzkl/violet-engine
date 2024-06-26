project(violet-core)

set(CORE_SOURCE
    ./private/engine_context.cpp
    ./private/engine_module.cpp
    ./private/engine.cpp
    ./private/plugin.cpp)

set(ECS_SOURCE
    ./private/ecs/actor.cpp
    ./private/ecs/archetype_chunk.cpp
    ./private/ecs/archetype.cpp
    ./private/ecs/view.cpp
    ./private/ecs/world.cpp)

add_library(${PROJECT_NAME} STATIC
    ${CORE_SOURCE}
    ${ECS_SOURCE})
add_library(violet::core ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./public
    PRIVATE
        ./private)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        violet::common
        violet::task)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)