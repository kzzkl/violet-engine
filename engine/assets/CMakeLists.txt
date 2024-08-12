project(violet-assets)

include(${VIOLET_ROOT_DIR}/cmake/compile_shader.cmake)
set(SHADER_FILES "")

set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders/source)

set(VS_SHADER_LIST
    "${SHADER_SOURCE_DIR}/ibl/cube_sample.hlsl"
    "${SHADER_SOURCE_DIR}/materials/basic.hlsl"
    "${SHADER_SOURCE_DIR}/materials/physical_tex.hlsl"
    "${SHADER_SOURCE_DIR}/materials/physical.hlsl"
    "${SHADER_SOURCE_DIR}/debug.hlsl"
    "${SHADER_SOURCE_DIR}/full_screen.hlsl"
    "${SHADER_SOURCE_DIR}/skybox.hlsl"
    "${SHADER_SOURCE_DIR}/ui.hlsl")

set(FS_SHADER_LIST
    "${SHADER_SOURCE_DIR}/ibl/brdf_lut.hlsl"
    "${SHADER_SOURCE_DIR}/ibl/equirectangular_to_cubemap.hlsl"
    "${SHADER_SOURCE_DIR}/ibl/irradiance.hlsl"
    "${SHADER_SOURCE_DIR}/ibl/prefilter.hlsl"
    "${SHADER_SOURCE_DIR}/materials/basic.hlsl"
    "${SHADER_SOURCE_DIR}/materials/physical_tex.hlsl"
    "${SHADER_SOURCE_DIR}/materials/physical.hlsl"
    "${SHADER_SOURCE_DIR}/debug.hlsl"
    "${SHADER_SOURCE_DIR}/skybox.hlsl"
    "${SHADER_SOURCE_DIR}/ui.hlsl")

foreach(SHADER_SOURCE ${VS_SHADER_LIST})
    compile_shader_dxc(
        SHADER_FILES
        SOURCE ${SHADER_SOURCE}
        STAGES "vs"
        INCLUDES "${VIOLET_SHADER_INCLUDE_DIR}")
endforeach()

foreach(SHADER_SOURCE ${FS_SHADER_LIST})
    compile_shader_dxc(
        SHADER_FILES
        SOURCE ${SHADER_SOURCE}
        STAGES "fs"
        INCLUDES "${VIOLET_SHADER_INCLUDE_DIR}")
endforeach()

add_custom_target(${PROJECT_NAME} ALL DEPENDS ${SHADER_FILES})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/config ${VIOLET_BINARY_ROOT_DIR}/assets/config
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fonts ${VIOLET_BINARY_ROOT_DIR}/assets/fonts
    COMMAND ${CMAKE_COMMAND} -E make_directory ${VIOLET_BINARY_ROOT_DIR}/assets/shaders
    COMMAND ${CMAKE_COMMAND} -E copy ${SHADER_FILES} ${VIOLET_BINARY_ROOT_DIR}/assets/shaders)