project(physics)

add_executable(${PROJECT_NAME}
    ./source/main.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        violet::graphics
        violet::physics
        violet::control)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

include(${VIOLET_ROOT_DIR}/cmake/compile_shader.cmake)
set(SHADER_FILES "")
compile_shader_dxc(
    SHADER_FILES
    TARGET ${PROJECT_NAME}
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/basic.hlsl"
    STAGES "vs" "fs")

install(FILES ${SHADER_FILES} DESTINATION bin/${PROJECT_NAME}/shaders)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION bin/${PROJECT_NAME})

if (MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()