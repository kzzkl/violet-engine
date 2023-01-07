project(violet-ecs)

add_library(${PROJECT_NAME} INTERFACE)
add_library(violet::ecs ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    INTERFACE
    ./include)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
    violet::common
    violet::core)

install(DIRECTORY ./include/ DESTINATION include/violet/ecs)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)