project(mmd-viewer)

add_executable(${PROJECT_NAME}
    ./source/encode.cpp
    ./source/main.cpp
    ./source/mmd_animation.cpp
    ./source/mmd_bezier.cpp
    ./source/mmd_loader.cpp
    ./source/mmd_pipeline.cpp
    ./source/mmd_viewer.cpp
    ./source/pmx_loader.cpp
    ./source/vmd_loader.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ash::ecs
        ash::core
        ash::window
        ash::graphics
        ash::math
        ash::scene
        ash::physics
        ash::editor)

include(${ASH_ROOT_DIR}/cmake/shader_compile.cmake)
set(SHADERS "")
list(APPEND SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/resource/shader/color.hlsl;vert;frag")
list(APPEND SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/resource/shader/edge.hlsl;vert;frag")
list(APPEND SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/resource/shader/skin.hlsl;comp")
target_shader_compile(
    "${PROJECT_NAME}"
    "${SHADERS}"
    "${CMAKE_CURRENT_SOURCE_DIR}/resource/shader/generated"
    "${ASH_THIRD_PARTY_DIR}/Vulkan/glslangValidator.exe")
target_shader_compile(
    "${PROJECT_NAME}"
    "${SHADERS}"
    "${CMAKE_CURRENT_SOURCE_DIR}/resource/shader/generated"
    "${ASH_THIRD_PARTY_DIR}/DirectX/dxc/dxc.exe")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource/config DESTINATION bin/resource)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource/mmd DESTINATION bin/resource)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource/model DESTINATION bin/resource)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource/shader/generated/ DESTINATION bin/resource/shader)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

if (MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()