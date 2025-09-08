add_library(violet-graphics STATIC
    private/geometries/box_geometry.cpp
    private/geometries/plane_geometry.cpp
    private/geometries/sphere_geometry.cpp
    private/materials/pbr_material.cpp
    private/materials/unlit_material.cpp
    private/render_graph/rdg_command.cpp
    private/render_graph/rdg_pass.cpp
    private/render_graph/render_graph.cpp
    private/renderers/passes/blit_pass.cpp
    private/renderers/passes/copy_depth_pass.cpp
    private/renderers/passes/cull_pass.cpp
    private/renderers/passes/debug/bounds_projection_pass.cpp
    private/renderers/passes/depth_only_pass.cpp
    private/renderers/passes/gbuffer_pass.cpp
    private/renderers/passes/gtao_pass.cpp
    private/renderers/passes/hzb_pass.cpp
    private/renderers/passes/mesh_pass.cpp
    private/renderers/passes/motion_vector_pass.cpp
    private/renderers/passes/scan_pass.cpp
    private/renderers/passes/shading_pass.cpp
    private/renderers/passes/skybox_pass.cpp
    private/renderers/passes/taa_pass.cpp
    private/renderers/passes/tone_mapping_pass.cpp
    private/renderers/deferred_renderer.cpp
    private/resources/brdf_lut.cpp
    private/resources/buffer.cpp
    private/resources/hilbert_lut.cpp
    private/resources/persistent_buffer.cpp
    private/resources/ramp_texture.cpp
    private/resources/texture.cpp
    private/shading_models/pbr_shading_model.cpp
    private/shading_models/unlit_shading_model.cpp
    private/tools/cluster/cluster_builder.cpp
    private/tools/cluster/graph_partitioner.cpp
    private/tools/mesh_simplifier/mesh_simplifier.cpp
    private/tools/mesh_simplifier/quadric.cpp
    private/tools/geometry_tool.cpp
    private/tools/ibl_tool.cpp
    private/tools/texture_loader.cpp
    private/camera_system.cpp
    private/debug_drawer.cpp
    private/environment_system.cpp
    private/geometry_manager.cpp
    private/geometry.cpp
    private/gpu_buffer_uploader.cpp
    private/graphics_config.cpp
    private/graphics_system.cpp
    private/light_system.cpp
    private/material_manager.cpp
    private/material.cpp
    private/mesh_system.cpp
    private/morph_target.cpp
    private/render_device.cpp
    private/render_scene_manager.cpp
    private/render_scene.cpp
    private/renderer.cpp
    private/rhi_plugin.cpp
    private/shader_compiler.cpp
    private/skinning_system.cpp
    private/skybox.cpp
    private/transient_allocator.cpp)
add_library(violet::graphics ALIAS violet-graphics)

target_include_directories(violet-graphics
    PUBLIC
        public
    PRIVATE
        private)

target_link_libraries(violet-graphics
    PUBLIC
        violet::algorithm
        violet::task
        violet::window
        violet::math
        violet::scene
    PRIVATE
        stb
        dxcompiler
        metis
        MikkTSpace)

install(TARGETS violet-graphics
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)