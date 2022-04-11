project(ash-scene)

add_library(${PROJECT_NAME} STATIC
    ./source/scene_node.cpp
    ./source/scene.cpp
    ./source/transform.cpp)
add_library(ash::scene ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ash::core
        ash::math)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)