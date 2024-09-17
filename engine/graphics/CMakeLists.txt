project(violet-graphics)

add_library(${PROJECT_NAME} STATIC
    ./private/geometries/box_geometry.cpp
    ./private/geometries/sphere_geometry.cpp
    ./private/materials/basic_material.cpp
    ./private/materials/physical_material.cpp
    ./private/passes/mesh_pass.cpp
    ./private/passes/skybox_pass.cpp
    ./private/render_graph/rdg_allocator.cpp
    ./private/render_graph/rdg_command.cpp
    ./private/render_graph/rdg_pass.cpp
    ./private/render_graph/rdg_resource.cpp
    ./private/render_graph/render_graph.cpp
    ./private/renderers/default_renderer.cpp
    ./private/tools/ibl_tool.cpp
    ./private/tools/texture_loader.cpp
    ./private/geometry.cpp
    ./private/graphics_system.cpp
    ./private/material.cpp
    ./private/render_context.cpp
    ./private/render_device.cpp
    ./private/renderer.cpp
    ./private/rhi_plugin.cpp)
add_library(violet::graphics ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./public
    PRIVATE
    ./private)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    stb
    violet::task
    violet::window
    violet::math
    violet::scene)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)