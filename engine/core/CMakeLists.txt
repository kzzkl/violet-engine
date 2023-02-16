project(violet-core)

add_library(${PROJECT_NAME} STATIC
    ./private/application.cpp
    ./private/context.cpp
    ./private/plugin.cpp
    ./private/relation.cpp)
add_library(violet::core ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./interface
    ./public)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    violet::common
    violet::ecs
    violet::task)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)