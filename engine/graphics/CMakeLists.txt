project(violet-graphics)

add_library(${PROJECT_NAME} STATIC
    ./private/components/camera.cpp
    ./private/components/mesh.cpp
    ./private/geometries/box_geometry.cpp
    ./private/geometries/sphere_geometry.cpp
    # ./private/material/basic_material.cpp
    # ./private/pipeline/basic_pipeline.cpp
    # ./private/pipeline/debug_pipeline.cpp
    # ./private/passes/skybox_pass.cpp
    # ./private/render_graph/compute_pass.cpp
    # ./private/render_graph/material.cpp
    ./private/render_graph/edge.cpp
    ./private/render_graph/material.cpp
    ./private/render_graph/pass_batch.cpp
    ./private/render_graph/pass.cpp
    ./private/render_graph/render_context.cpp
    ./private/render_graph/render_graph.cpp
    ./private/render_graph/resource.cpp
    # ./private/render_graph/render_pass.cpp
    # ./private/render_graph/render_pipeline.cpp
    ./private/geometry.cpp
    ./private/graphics_system.cpp
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
    violet::task
    violet::window
    violet::math
    violet::scene)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

include(${VIOLET_ROOT_DIR}/cmake/compile_shader.cmake)
set(SHADER_FILES "")
compile_shader_dxc(
    SHADER_FILES
    TARGET ${PROJECT_NAME}
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/shaders/hlsl/debug.hlsl"
    STAGES "vert" "frag"
    INCLUDES "${VIOLET_SHADER_INCLUDE_DIR}")
compile_shader_dxc(
    SHADER_FILES
    TARGET ${PROJECT_NAME}
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/shaders/hlsl/skybox.hlsl"
    STAGES "vert" "frag"
    INCLUDES "${VIOLET_SHADER_INCLUDE_DIR}")

install(FILES ${SHADER_FILES} DESTINATION bin/engine/shaders)