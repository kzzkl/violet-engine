project(ash-core)

add_library(${PROJECT_NAME} STATIC
    ./source/application.cpp
    ./source/context.cpp
    ./source/plugin.cpp)
add_library(ash::core ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include
        ./interface)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ash::common
        ash::ecs
        ash::task)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)