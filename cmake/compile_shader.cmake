# dxc
function(compile_shader_dxc OUTPUT_FILES)
    set(DXC_PATH "dxc.exe")
    set(DXC_WORKING_DIRECTORY "${VIOLET_THIRD_PARTY_DIR}/dxc")

    set(oneValueArgs SOURCE)
    set(multiValueArgs STAGES INCLUDES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGV})

    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)
    file(MAKE_DIRECTORY ${OUTPUT_DIR})

    get_filename_component(SHADER_NAME ${ARG_SOURCE} NAME_WE)
    foreach(STAGE ${ARG_STAGES})
        if(STAGE STREQUAL "vs")
            set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.vs")
            set(PROFILE "vs_6_0")
            set(ENTRY_POINT "vs_main")
        elseif(STAGE STREQUAL "fs")
            set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.fs")
            set(PROFILE "ps_6_0")
            set(ENTRY_POINT "fs_main")
        elseif(STAGE STREQUAL "cs")
            set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.cs")
            set(PROFILE "cs_6_0")
            set(ENTRY_POINT "cs_main")
        endif()

        set(COMMAND_LINE ${DXC_PATH} -spirv ${ARG_SOURCE} -T ${PROFILE} -E ${ENTRY_POINT} -Fo ${OUTPUT_NAME} -Wno-ignored-attributes -all-resources-bound $<$<CONFIG:Debug>:-Zi>)

        foreach(INCLUDE ${ARG_INCLUDES})
            set(COMMAND_LINE ${COMMAND_LINE} -I ${INCLUDE})
        endforeach()

        add_custom_command(
            OUTPUT ${OUTPUT_NAME}
            COMMAND ${COMMAND_LINE}
            DEPENDS ${ARG_SOURCE}
            WORKING_DIRECTORY ${DXC_WORKING_DIRECTORY}
            COMMAND_EXPAND_LISTS)

        list(APPEND ${OUTPUT_FILES} ${OUTPUT_NAME})
    endforeach()

    return(PROPAGATE ${OUTPUT_FILES})

endfunction()