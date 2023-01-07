project(mmd-viewer)

add_executable(${PROJECT_NAME}
    ./source/encode.cpp
    ./source/main.cpp
    ./source/mmd_animation.cpp
    ./source/mmd_bezier.cpp
    ./source/mmd_component.cpp
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
    violet::ecs
    violet::core
    violet::window
    violet::graphics
    violet::math
    violet::scene
    violet::physics
    violet::editor)

include(${VIOLET_ROOT_DIR}/cmake/shader_compile.cmake)
target_shader_compile(
    "${PROJECT_NAME}"
    "${CMAKE_CURRENT_SOURCE_DIR}/resource/shader/color.hlsl"
    "vert;frag"
    "${VIOLET_SHADER_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/resource/shader/generated")
target_shader_compile(
    "${PROJECT_NAME}"
    "${CMAKE_CURRENT_SOURCE_DIR}/resource/shader/edge.hlsl"
    "vert;frag"
    "${VIOLET_SHADER_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/resource/shader/generated")
target_shader_compile(
    "${PROJECT_NAME}"
    "${CMAKE_CURRENT_SOURCE_DIR}/resource/shader/skin.hlsl"
    "comp"
    ""
    "${CMAKE_CURRENT_SOURCE_DIR}/resource/shader/generated")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource/config DESTINATION bin/mmd-viewer)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource/mmd DESTINATION bin/mmd-viewer)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resource/shader/generated/ DESTINATION bin/mmd-viewer/shader)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()