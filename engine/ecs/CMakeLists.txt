project(violet-ecs)

add_library(${PROJECT_NAME} STATIC
    ./private/actor.cpp
    ./private/archetype_chunk.cpp
    ./private/archetype.cpp
    ./private/view.cpp
    ./private/world.cpp)
add_library(violet::ecs ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./public
    PRIVATE
        ./private)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        violet::common)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)