project(violet-sample-base)

add_library(imgui STATIC
    ${VIOLET_THIRDPARTY_DIR}/imgui/imgui.cpp
    ${VIOLET_THIRDPARTY_DIR}/imgui/imgui_demo.cpp
    ${VIOLET_THIRDPARTY_DIR}/imgui/imgui_draw.cpp
    ${VIOLET_THIRDPARTY_DIR}/imgui/imgui_tables.cpp
    ${VIOLET_THIRDPARTY_DIR}/imgui/imgui_widgets.cpp)
target_include_directories(imgui
    PUBLIC
    ${VIOLET_THIRDPARTY_DIR}/imgui)

add_library(${PROJECT_NAME} STATIC
    ./private/deferred_renderer_imgui.cpp
    ./private/gltf_loader.cpp
    ./private/imgui_system.cpp)
add_library(violet::sample-base ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./public
    PRIVATE
    ./private)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    violet::graphics
    assimp::assimp
    MikkTSpace
    imgui)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION bin/assets)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)