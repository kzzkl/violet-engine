set(DXC_PATH "${CMAKE_SOURCE_DIR}/engine/shader/compiler/DirectX/dxc/dxc.exe")
set(DXC_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/engine/shader/compiler/DirectX/dxc")

set(GLSLANG_PATH "${CMAKE_SOURCE_DIR}/engine/shader/compiler/Vulkan/glslangValidator.exe")
set(GLSLANG_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/engine/shader/compiler/Vulkan")

# dxc
function(compile_shader_dxc TARGET_NAME SHADER_PATH TYPE INCLUDE_DIR OUTPUT_DIR)
    get_filename_component(SHADER_NAME ${SHADER_PATH} NAME_WE)

    if(TYPE STREQUAL "vert")
        set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.vert.cso")
        set(PROFILE "vs_6_0")
        set(ENTRY_POINT "vs_main")
    elseif(TYPE STREQUAL "frag")
        set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.frag.cso")
        set(PROFILE "ps_6_0")
        set(ENTRY_POINT "ps_main")
    elseif(TYPE STREQUAL "comp")
        set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.comp.cso")
        set(PROFILE "cs_6_0")
        set(ENTRY_POINT "cs_main")
    endif()

    set(DXC_CMD_DEBUG ${DXC_PATH} ${SHADER_PATH} -T ${PROFILE} -E ${ENTRY_POINT} -Fo ${OUTPUT_NAME} -Wno-ignored-attributes -all-resources-bound -Zi -Qembed_debug)
    set(DXC_CMD_RELEASE ${DXC_PATH} ${SHADER_PATH} -T ${PROFILE} -E ${ENTRY_POINT} -Fo ${OUTPUT_NAME} -Wno-ignored-attributes -all-resources-bound)

    if (NOT INCLUDE_DIR STREQUAL "")
        set(DXC_CMD_DEBUG ${DXC_CMD_DEBUG} -I ${INCLUDE_DIR})
    endif()

    add_custom_command(
        TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND "$<$<CONFIG:Debug>:${DXC_CMD_DEBUG}>$<$<CONFIG:Release>:${DXC_CMD_RELEASE}>"
        DEPENDS ${SHADER_PATH}
        WORKING_DIRECTORY "${DXC_WORKING_DIRECTORY}"
        COMMAND_EXPAND_LISTS)
endfunction()

# glslang
function(compile_shader_glslang TARGET_NAME SHADER_PATH TYPE INCLUDE_DIR OUTPUT_DIR)
    get_filename_component(SHADER_NAME ${SHADER_PATH} NAME_WE)

    if(TYPE STREQUAL "vert")
        set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.vert.spv")
        set(PROFILE "vert")
        set(ENTRY_POINT "vs_main")
    elseif(TYPE STREQUAL "frag")
        set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.frag.spv")
        set(PROFILE "frag")
        set(ENTRY_POINT "ps_main")
    elseif(TYPE STREQUAL "comp")
        set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.comp.spv")
        set(PROFILE "comp")
        set(ENTRY_POINT "cs_main")
    endif()

    add_custom_command(
        TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND ${GLSLANG_PATH} -D --client vulkan100 -S ${PROFILE} -e ${ENTRY_POINT} ${SHADER_PATH} -o ${OUTPUT_NAME} --auto-sampled-textures
        DEPENDS ${SHADER_PATH}
        WORKING_DIRECTORY "${GLSLANG_WORKING_DIRECTORY}")
endfunction()

function(test TARGET_NAME SHADER_PATH TYPE INCLUDE_DIR)
    message("${TARGET_NAME} ${SHADER_PATH} ${TYPE}")
endfunction()

function(target_shader_compile TARGET_NAME SHADER_PATH TYPES INCLUDE_DIR OUTPUT_DIR)
    foreach(TYPE ${TYPES})
        compile_shader_dxc(${TARGET_NAME} "${SHADER_PATH}" ${TYPE} "${INCLUDE_DIR}" "${OUTPUT_DIR}")
        #compile_shader_glslang(${TARGET_NAME} "${SHADER_PATH}" ${TYPE} "${INCLUDE_DIR}" "${OUTPUT_DIR}")
    endforeach()
endfunction()