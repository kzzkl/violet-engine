project(violet-scene)

add_library(${PROJECT_NAME} STATIC
    ./private/hierarchy_system.cpp
    ./private/scene_system.cpp
    ./private/transform_system.cpp)
add_library(violet::scene ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./public)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    violet::core
    violet::math
    violet::ecs-command)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)