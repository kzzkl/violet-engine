project(ash-ui)

set(IMGUI_INCLUDE ${THIRD_PARTY_DIR}/../imgui)
set(IMGUI_SOURCE
    ${THIRD_PARTY_DIR}/../imgui/imgui.cpp
    ${THIRD_PARTY_DIR}/../imgui/imgui_demo.cpp
    ${THIRD_PARTY_DIR}/../imgui/imgui_draw.cpp
    ${THIRD_PARTY_DIR}/../imgui/imgui_tables.cpp
    ${THIRD_PARTY_DIR}/../imgui/imgui_widgets.cpp)

add_library(${PROJECT_NAME} STATIC
    ./source/ui_pipeline.cpp
    ./source/ui.cpp
    ${IMGUI_SOURCE})
add_library(ash::ui ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include
    PRIVATE
        ${IMGUI_INCLUDE})

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ash::common
        ash::core
        ash::graphics)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)