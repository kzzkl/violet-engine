function(target_shader_compile TARGET_NAME PARAMETERS OUTPUT_DIR COMPILER_EXECUTABLE)
    get_filename_component(COMPILER_NAME ${COMPILER_EXECUTABLE} NAME_WE)
    get_filename_component(WORKING_DIR ${COMPILER_EXECUTABLE} DIRECTORY)
    set(SHADER_PATH "")
    set(SHADER_NAME "")
    foreach(PARAMETER ${PARAMETERS})
        string(FIND ${PARAMETER} ".hlsl" IS_SHADER)
        if(${IS_SHADER} EQUAL -1)
            set(OUTPUT_NAME "")
            set(PROFILE "")
            set(ENTRY_POINT "")
            if(COMPILER_NAME STREQUAL "dxc")
                if(PARAMETER STREQUAL "vert")
                    set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.vert.cso")
                    set(PROFILE "vs_6_0")
                    set(ENTRY_POINT "vs_main")
                elseif(PARAMETER STREQUAL "frag")
                    set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.frag.cso")
                    set(PROFILE "ps_6_0")
                    set(ENTRY_POINT "ps_main")
                elseif(PARAMETER STREQUAL "comp")
                    set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.comp.cso")
                    set(PROFILE "cs_6_0")
                    set(ENTRY_POINT "cs_main")
                endif()

                set(DXC_CMD_DEBUG ${COMPILER_EXECUTABLE} ${SHADER_PATH} -T ${PROFILE} -E ${ENTRY_POINT} -Fo ${OUTPUT_NAME} -Wno-ignored-attributes -all-resources-bound -Zi -Qembed_debug)
                set(DXC_CMD_RELEASE ${COMPILER_EXECUTABLE} ${SHADER_PATH} -T ${PROFILE} -E ${ENTRY_POINT} -Fo ${OUTPUT_NAME} -Wno-ignored-attributes -all-resources-bound)
                add_custom_command(
                    TARGET ${TARGET_NAME}
                    POST_BUILD
                    COMMAND "$<$<CONFIG:Debug>:${DXC_CMD_DEBUG}>$<$<CONFIG:Release>:${DXC_CMD_RELEASE}>"
                    DEPENDS ${SHADER_PATH}
                    WORKING_DIRECTORY "${WORKING_DIR}"
                    COMMAND_EXPAND_LISTS)
            elseif(COMPILER_NAME STREQUAL "glslangValidator")
                if(PARAMETER STREQUAL "vert")
                    set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.vert.spv")
                    set(PROFILE "vert")
                    set(ENTRY_POINT "vs_main")
                elseif(PARAMETER STREQUAL "frag")
                    set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.frag.spv")
                    set(PROFILE "frag")
                    set(ENTRY_POINT "ps_main")
                elseif(PARAMETER STREQUAL "comp")
                    set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.comp.spv")
                    set(PROFILE "comp")
                    set(ENTRY_POINT "cs_main")
                endif()
                add_custom_command(
                    TARGET ${TARGET_NAME}
                    POST_BUILD
                    COMMAND ${COMPILER_EXECUTABLE} -D --client vulkan100 -S ${PROFILE} -e ${ENTRY_POINT} ${SHADER_PATH} -o ${OUTPUT_NAME} --auto-sampled-textures
                    DEPENDS ${SHADER_PATH}
                    WORKING_DIRECTORY "${WORKING_DIR}")
            endif()
        else()
            set(SHADER_PATH ${PARAMETER})
            get_filename_component(SHADER_NAME ${SHADER_PATH} NAME_WE)
        endif()
    endforeach()
endfunction()