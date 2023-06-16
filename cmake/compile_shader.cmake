set(DXC_PATH "${VIOLET_THIRD_PARTY_DIR}/dxc/dxc.exe")
set(DXC_WORKING_DIRECTORY "${VIOLET_THIRD_PARTY_DIR}/dxc")

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

    if(NOT INCLUDE_DIR STREQUAL "")
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

function(compile_shader_glslang)
    set(GLSLANG_PATH "${VIOLET_THIRD_PARTY_DIR}/glslang/glslangValidator.exe")
    set(GLSLANG_WORKING_DIRECTORY "${VIOLET_THIRD_PARTY_DIR}/glslang")

    set(oneValueArgs TARGET SOURCE)
    set(multiValueArgs STAGES INCLUDE_DIRS)

    cmake_parse_arguments(COMPILE_SHADER "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGV})

    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders/${COMPILE_SHADER_TARGET})
    file(MAKE_DIRECTORY ${OUTPUT_DIR})

    get_filename_component(SHADER_NAME ${COMPILE_SHADER_SOURCE} NAME_WE)
    foreach(STAGE ${COMPILE_SHADER_STAGES})
        if(STAGE STREQUAL "vert")
            set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.vert.spv")
            set(ENTRY_POINT "vs_main")
        elseif(STAGE STREQUAL "frag")
            set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.frag.spv")
            set(ENTRY_POINT "ps_main")
        elseif(STAGE STREQUAL "comp")
            set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.comp.spv")
            set(ENTRY_POINT "cs_main")
        endif()

        set(COMMAND_LINE ${GLSLANG_PATH} -D --client vulkan100 -S ${STAGE} -e ${ENTRY_POINT} ${COMPILE_SHADER_SOURCE} -o ${OUTPUT_NAME} --auto-sampled-textures)
        foreach(INCLUDE ${COMPILE_SHADER_INCLUDE_DIRS})
            set(COMMAND_LINE ${COMMAND_LINE} -I${INCLUDE})
        endforeach()

        add_custom_command(
            TARGET ${COMPILE_SHADER_TARGET}
            # OUTPUT ${OUTPUT_NAME}
            POST_BUILD
            COMMAND ${COMMAND_LINE}
            DEPENDS ${COMPILE_SHADER_SOURCE}
            WORKING_DIRECTORY "${GLSLANG_WORKING_DIRECTORY}")

    endforeach()

endfunction()