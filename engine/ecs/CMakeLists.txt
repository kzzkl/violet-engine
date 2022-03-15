project(ash-ecs)

add_library(${PROJECT_NAME} INTERFACE)
add_library(ash::ecs ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    INTERFACE
        ./include)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        ash::common)

install(DIRECTORY ./include/ DESTINATION include/ash/ecs)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)