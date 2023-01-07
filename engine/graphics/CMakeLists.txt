project(violet-graphics)

add_library(${PROJECT_NAME} STATIC
    ./source/blinn_phong_pipeline.cpp
    ./source/camera.cpp
    ./source/geometry.cpp
    ./source/graphics_debug.cpp
    ./source/graphics.cpp
    ./source/light.cpp
    ./source/mesh_render.cpp
    ./source/pipeline_parameter.cpp
    ./source/render_pipeline.cpp
    ./source/rhi.cpp
    ./source/shadow_map.cpp
    ./source/shadow_pipeline.cpp
    ./source/skinning_pipeline.cpp
    ./source/sky_pipeline.cpp)
add_library(violet::graphics ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./include
    ./interface)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    violet::core
    violet::window
    violet::math
    violet::scene)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)