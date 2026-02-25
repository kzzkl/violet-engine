add_library(imgui STATIC
    ${VIOLET_THIRDPARTY_DIR}/imgui/imgui.cpp
    ${VIOLET_THIRDPARTY_DIR}/imgui/imgui_demo.cpp
    ${VIOLET_THIRDPARTY_DIR}/imgui/imgui_draw.cpp
    ${VIOLET_THIRDPARTY_DIR}/imgui/imgui_tables.cpp
    ${VIOLET_THIRDPARTY_DIR}/imgui/imgui_widgets.cpp)
target_include_directories(imgui
    PUBLIC
        ${VIOLET_THIRDPARTY_DIR}/imgui)

add_library(violet-sample-base STATIC
    private/assimp_loader.cpp
    private/gltf_loader.cpp
    private/imgui_pass.cpp
    private/imgui_system.cpp
    private/sample_system.cpp)
add_library(violet::sample-base ALIAS violet-sample-base)

target_include_directories(violet-sample-base
    PUBLIC
        public
    PRIVATE
        private)

target_link_libraries(violet-sample-base
    PUBLIC
        violet::control
        violet::graphics
        assimp::assimp
        imgui
        stb)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION bin/assets)

install(TARGETS violet-sample-base
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)