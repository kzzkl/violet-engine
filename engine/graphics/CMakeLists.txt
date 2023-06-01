project(violet-graphics)

add_library(${PROJECT_NAME} STATIC
    ./private/components/mesh.cpp
    # ./private/material/basic_material.cpp
    ./private/render_graph/node_parameter.cpp
    ./private/render_graph/pipeline_parameter.cpp
    ./private/render_graph/render_graph.cpp
    ./private/render_graph/render_pipeline.cpp
    ./private/rhi_plugin.cpp
    ./private/graphics_module.cpp)
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