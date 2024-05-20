project(mmd-viewer)

add_executable(${PROJECT_NAME}
    ./source/components/mmd_animator.cpp
    ./source/components/mmd_morph.cpp
    ./source/components/mmd_skeleton.cpp
    ./source/bezier.cpp
    ./source/encode.cpp
    ./source/main.cpp
    ./source/mmd_animation.cpp
    ./source/mmd_loader.cpp
    ./source/mmd_render.cpp
    ./source/mmd_viewer.cpp
    ./source/pmx.cpp
    ./source/vmd.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    violet::graphics
    violet::physics
    violet::control)

include(${VIOLET_ROOT_DIR}/cmake/compile_shader.cmake)
set(SHADER_FILES "")
compile_shader_dxc(
    SHADER_FILES
    TARGET ${PROJECT_NAME}
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/color.hlsl"
    STAGES "vert" "frag"
    INCLUDES "${VIOLET_SHADER_INCLUDE_DIR}")
compile_shader_dxc(
    SHADER_FILES
    TARGET ${PROJECT_NAME}
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/edge.hlsl"
    STAGES "vert" "frag"
    INCLUDES "${VIOLET_SHADER_INCLUDE_DIR}")
compile_shader_dxc(
    SHADER_FILES
    TARGET ${PROJECT_NAME}
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/skinning.hlsl"
    STAGES "comp"
    INCLUDES "${VIOLET_SHADER_INCLUDE_DIR}")

install(FILES ${SHADER_FILES} DESTINATION bin/${PROJECT_NAME}/shaders)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION bin/${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()