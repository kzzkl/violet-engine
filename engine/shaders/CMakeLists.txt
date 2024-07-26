project(violet-shaders)

include(${VIOLET_ROOT_DIR}/cmake/compile_shader.cmake)
set(SHADER_FILES "")

set(VS_SHADER_LIST
    "${CMAKE_CURRENT_SOURCE_DIR}/source/ibl/cube_sample.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/materials/basic.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/materials/physical_tex.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/materials/physical.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/debug.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/full_screen.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/skybox.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/ui.hlsl")

set(FS_SHADER_LIST
    "${CMAKE_CURRENT_SOURCE_DIR}/source/ibl/brdf_lut.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/ibl/equirectangular_to_cubemap.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/ibl/irradiance.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/ibl/prefilter.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/materials/basic.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/materials/physical_tex.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/materials/physical.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/debug.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/skybox.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/ui.hlsl")

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

install(FILES ${SHADER_FILES} DESTINATION bin/engine/shaders)