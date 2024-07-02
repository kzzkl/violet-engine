# dxc
function(compile_shader_dxc OUTPUT_FILES)
    set(DXC_PATH "${VIOLET_THIRD_PARTY_DIR}/dxc/dxc.exe")
    set(DXC_WORKING_DIRECTORY "${VIOLET_THIRD_PARTY_DIR}/dxc")

    set(oneValueArgs SOURCE)
    set(multiValueArgs STAGES INCLUDES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGV})

    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)
    file(MAKE_DIRECTORY ${OUTPUT_DIR})

    get_filename_component(SHADER_NAME ${ARG_SOURCE} NAME_WE)
    foreach(STAGE ${ARG_STAGES})
        if(STAGE STREQUAL "vert")
            set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.vert.spv")
            set(PROFILE "vs_6_0")
            set(ENTRY_POINT "vs_main")
        elseif(STAGE STREQUAL "frag")
            set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.frag.spv")
            set(PROFILE "ps_6_0")
            set(ENTRY_POINT "ps_main")
        elseif(STAGE STREQUAL "comp")
            set(OUTPUT_NAME "${OUTPUT_DIR}/${SHADER_NAME}.comp.spv")
            set(PROFILE "cs_6_0")
            set(ENTRY_POINT "cs_main")
        endif()

        set(COMMAND_LINE_DEBUG ${DXC_PATH} -spirv ${ARG_SOURCE} -T ${PROFILE} -E ${ENTRY_POINT} -Fo ${OUTPUT_NAME} -Wno-ignored-attributes -all-resources-bound -Zi -Qembed_debug)
        set(COMMAND_LINE_RELEASE ${DXC_PATH} -spirv ${ARG_SOURCE} -T ${PROFILE} -E ${ENTRY_POINT} -Fo ${OUTPUT_NAME} -Wno-ignored-attributes -all-resources-bound)

        set(COMMAND_LINE "")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(COMMAND_LINE ${COMMAND_LINE_DEBUG})
        elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
            set(COMMAND_LINE ${COMMAND_LINE_RELEASE})
        endif()

        foreach(INCLUDE ${ARG_INCLUDES})
            set(COMMAND_LINE ${COMMAND_LINE} -I${INCLUDE})
        endforeach()

        add_custom_command(
            OUTPUT ${OUTPUT_NAME}
            COMMAND ${COMMAND_LINE}
            DEPENDS ${ARG_SOURCE}
            WORKING_DIRECTORY "${DXC_WORKING_DIRECTORY}"
            COMMAND_EXPAND_LISTS)

        list(APPEND ${OUTPUT_FILES} ${OUTPUT_NAME})
    endforeach()

    return(PROPAGATE ${OUTPUT_FILES})

endfunction()