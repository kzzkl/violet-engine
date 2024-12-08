project(violet-graphics)

add_library(${PROJECT_NAME} STATIC
    ./private/geometries/box_geometry.cpp
    ./private/geometries/sphere_geometry.cpp
    ./private/materials/physical_material.cpp
    ./private/materials/unlit_material.cpp
    ./private/passes/lighting/physical_pass.cpp
    ./private/passes/lighting/unlit_pass.cpp
    ./private/passes/blit_pass.cpp
    ./private/passes/copy_depth_pass.cpp
    ./private/passes/cull_pass.cpp
    ./private/passes/mesh_pass.cpp
    ./private/passes/skybox_pass.cpp
    ./private/passes/tone_mapping_pass.cpp
    ./private/render_graph/rdg_allocator.cpp
    ./private/render_graph/rdg_command.cpp
    ./private/render_graph/rdg_pass.cpp
    ./private/render_graph/rdg_resource.cpp
    ./private/render_graph/render_graph.cpp
    ./private/renderers/deferred_renderer.cpp
    ./private/tools/ibl_tool.cpp
    ./private/tools/texture_loader.cpp
    ./private/geometry_manager.cpp
    ./private/geometry.cpp
    ./private/gpu_buffer_uploader.cpp
    ./private/graphics_system.cpp
    ./private/material_manager.cpp
    ./private/material.cpp
    ./private/render_device.cpp
    ./private/render_scene.cpp
    ./private/rhi_plugin.cpp
    ./private/shader_compiler.cpp)
add_library(violet::graphics ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./public
    PRIVATE
    ./private)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    violet::task
    violet::window
    violet::math
    violet::scene
    PRIVATE
    stb
    dxcompiler)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)