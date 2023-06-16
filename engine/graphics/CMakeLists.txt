project(violet-graphics)

add_library(${PROJECT_NAME} STATIC
    ./private/components/mesh.cpp
    # ./private/material/basic_material.cpp
    ./private/render_graph/render_graph.cpp
    ./private/render_graph/render_pass.cpp
    ./private/render_graph/render_pipeline.cpp
    ./private/geometry.cpp
    ./private/graphics_module.cpp
    ./private/node_parameter.cpp
    ./private/pipeline_parameter.cpp
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

# include(${VIOLET_ROOT_DIR}/cmake/compile_shader.cmake)
# 
# add_custom_target("graphics_shader_generater" ALL)
# 
# set(SHADERS "")
# list(APPEND SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/blinn_phong.hlsl" "vert;frag")
# list(APPEND SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/debug.hlsl;vert;frag")
# list(APPEND SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/shadow.hlsl;vert")
# list(APPEND SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/sky.hlsl;vert;frag")
# list(APPEND SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/ui.hlsl;vert;frag")
# target_compile_shader(
#     "graphics_shader_generater"
#     "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/blinn_phong.hlsl"
#     "vert;frag"
#     "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/include"
#     "${CMAKE_CURRENT_SOURCE_DIR}/generated")
# target_compile_shader(
#     "graphics_shader_generater"
#     "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/debug.hlsl"
#     "vert;frag"
#     "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/include"
#     "${CMAKE_CURRENT_SOURCE_DIR}/generated")
# target_compile_shader(
#     "graphics_shader_generater"
#     "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/shadow.hlsl"
#     "vert"
#     "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/include"
#     "${CMAKE_CURRENT_SOURCE_DIR}/generated")
# target_compile_shader(
#     "graphics_shader_generater"
#     "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/sky.hlsl"
#     "vert;frag"
#     "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/include"
#     "${CMAKE_CURRENT_SOURCE_DIR}/generated")
# target_compile_shader(
#     "graphics_shader_generater"
#     "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/ui.hlsl"
#     "vert;frag"
#     "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/include"
#     "${CMAKE_CURRENT_SOURCE_DIR}/generated")
# 
# install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/generated/ DESTINATION bin/engine/shader)