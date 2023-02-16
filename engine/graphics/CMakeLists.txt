project(violet-graphics)

add_library(${PROJECT_NAME} STATIC
    ./private/blinn_phong_pipeline.cpp
    ./private/camera.cpp
    ./private/geometry.cpp
    ./private/graphics_debug.cpp
    ./private/graphics.cpp
    ./private/light.cpp
    ./private/mesh_render.cpp
    ./private/pipeline.cpp
    ./private/render_pipeline.cpp
    ./private/rhi.cpp
    ./private/shadow_map.cpp
    ./private/shadow_pipeline.cpp
    ./private/skinning_pipeline.cpp
    ./private/sky_pipeline.cpp)
add_library(violet::graphics ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./interface
    ./public)

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