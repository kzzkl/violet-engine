project(ash-graphics)

add_library(${PROJECT_NAME} STATIC
    ./source/geometry.cpp
    ./source/graphics_config.cpp
    ./source/graphics_debug.cpp
    ./source/graphics_interface_helper.cpp
    ./source/graphics_plugin.cpp
    ./source/graphics.cpp
    ./source/render_pipeline.cpp)
add_library(ash::graphics ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include
        ./interface)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ash::core
        ash::window
        ash::math
        ash::scene)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)