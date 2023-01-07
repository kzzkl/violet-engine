project(violet-ui)

# Build yoga layout engine.
file(GLOB_RECURSE VIOLET_YOGA_SOURCE ${VIOLET_THIRD_PARTY_DIR}/yoga/yoga/*.cpp)
add_library(violet-yoga STATIC ${VIOLET_YOGA_SOURCE})
target_include_directories(violet-yoga PUBLIC ${VIOLET_THIRD_PARTY_DIR}/yoga/.)
add_library(violet::yoga ALIAS violet-yoga)

# Build ui system.
add_library(${PROJECT_NAME} STATIC
    ./source/controls/button.cpp
    ./source/controls/collapse.cpp
    ./source/controls/dock_area.cpp
    ./source/controls/dock_element.cpp
    ./source/controls/dock_window.cpp
    ./source/controls/font_icon.cpp
    ./source/controls/image.cpp
    ./source/controls/input.cpp
    ./source/controls/label.cpp
    ./source/controls/panel.cpp
    ./source/controls/scroll_view.cpp
    ./source/controls/tree.cpp
    ./source/controls/view.cpp
    ./source/element_layout.cpp
    ./source/element_tree.cpp
    ./source/element.cpp
    ./source/font.cpp
    ./source/renderer.cpp
    ./source/ui_pipeline.cpp
    ./source/ui.cpp)
add_library(violet::ui ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./include)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    violet::common
    violet::core
    violet::graphics
    PRIVATE
    freetype
    violet::yoga)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)