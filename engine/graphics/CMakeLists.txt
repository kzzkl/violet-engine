project(violet-graphics)

add_library(${PROJECT_NAME} STATIC
    ./private/components/camera.cpp
    ./private/components/mesh.cpp
    # ./private/material/basic_material.cpp
    ./private/pipeline/basic_pipeline.cpp
    ./private/render_graph/material.cpp
    ./private/render_graph/render_graph.cpp
    ./private/render_graph/render_pass.cpp
    ./private/render_graph/render_pipeline.cpp
    ./private/render_graph/render_resource.cpp
    ./private/geometry.cpp
    ./private/graphics_system.cpp
    ./private/rhi_plugin.cpp)
add_library(violet::graphics ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./public
    PRIVATE
    ./private)

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