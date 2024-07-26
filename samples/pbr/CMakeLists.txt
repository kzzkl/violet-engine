project(pbr)

add_executable(${PROJECT_NAME}
    ./source/fbx_loader.cpp
    ./source/main.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ./include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    violet::graphics
    violet::control
    assimp::assimp)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

include(${VIOLET_ROOT_DIR}/cmake/compile_shader.cmake)
set(SHADER_FILES "")

set(SHADER_LIST
    # "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/pbr.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/test.hlsl")

foreach(SHADER_SOURCE ${SHADER_LIST})
    compile_shader_dxc(
        SHADER_FILES
        SOURCE ${SHADER_SOURCE}
        STAGES "vs" "fs"
        INCLUDES "${VIOLET_SHADER_INCLUDE_DIR}")
endforeach()

add_custom_target("pbr-shaders" ALL DEPENDS ${SHADER_FILES})

install(FILES ${SHADER_FILES} DESTINATION bin/${PROJECT_NAME}/shaders)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION bin/${PROJECT_NAME})

if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()