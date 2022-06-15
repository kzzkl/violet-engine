project(ash-editor-ui)

add_library(${PROJECT_NAME}
    ./source/component_view.cpp
    ./source/editor_ui.cpp
    ./source/hierarchy_view.cpp
    ./source/scene_view.cpp)
add_library(ash::editor::ui ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ash::ui)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)