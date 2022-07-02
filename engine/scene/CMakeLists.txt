project(ash-scene)

add_library(${PROJECT_NAME} STATIC
    ./source/bounding_box.cpp
    ./source/bvh_tree.cpp
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