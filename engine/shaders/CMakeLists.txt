project(violet-shaders)

# add_custom_target(${PROJECT_NAME})

include(${VIOLET_ROOT_DIR}/cmake/compile_shader.cmake)
set(SHADER_FILES "")

set(SHADER_LIST
    "${CMAKE_CURRENT_SOURCE_DIR}/source/materials/basic.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/debug.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/skybox.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/ui.hlsl")

foreach(SHADER_SOURCE ${SHADER_LIST})
    compile_shader_dxc(
        SHADER_FILES
        TARGET ${PROJECT_NAME}
        SOURCE ${SHADER_SOURCE}
        STAGES "vert" "frag"
        INCLUDES "${VIOLET_SHADER_INCLUDE_DIR}")
endforeach()

add_custom_target(${PROJECT_NAME} ALL DEPENDS ${SHADER_FILES})

install(FILES ${SHADER_FILES} DESTINATION bin/engine/shaders)