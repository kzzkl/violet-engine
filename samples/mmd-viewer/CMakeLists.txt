project(mmd-viewer)

add_executable(${PROJECT_NAME}
    ./source/encode.cpp
    ./source/main.cpp
    ./source/mmd_render.cpp
    ./source/mmd_viewer.cpp
    ./source/pmx_loader.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    violet::graphics
    violet::physics
    violet::control)

include(${VIOLET_ROOT_DIR}/cmake/compile_shader.cmake)
compile_shader_dxc(
    TARGET ${PROJECT_NAME}
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/color.hlsl"
    STAGES "vert" "frag"
    INCLUDES "${VIOLET_SHADER_INCLUDE_DIR}")

file(GLOB SHADER_FILES "${CMAKE_BINARY_DIR}/shaders/${PROJECT_NAME}/*.spv")
install(FILES ${SHADER_FILES} DESTINATION bin/${PROJECT_NAME}/shaders)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION bin/${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()