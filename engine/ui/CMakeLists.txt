project(violet-ui)

# Build yoga layout engine.
file(GLOB_RECURSE VIOLET_YOGA_SOURCE ${VIOLET_THIRD_PARTY_DIR}/yoga/yoga/*.cpp)
add_library(violet-yoga STATIC ${VIOLET_YOGA_SOURCE})
target_include_directories(violet-yoga PUBLIC ${VIOLET_THIRD_PARTY_DIR}/yoga/.)
add_library(violet::yoga ALIAS violet-yoga)

# Build ui system.
add_library(${PROJECT_NAME} STATIC
    ./private/controls/button.cpp
    ./private/controls/collapse.cpp
    ./private/controls/dock_area.cpp
    ./private/controls/dock_node.cpp
    ./private/controls/dock_window.cpp
    ./private/controls/font_icon.cpp
    ./private/controls/image.cpp
    ./private/controls/input.cpp
    ./private/controls/label.cpp
    ./private/controls/panel.cpp
    ./private/controls/scroll_view.cpp
    ./private/controls/tree.cpp
    ./private/controls/view.cpp
    ./private/event/event_node.cpp
    ./private/layout/layout_node_yoga.cpp
    ./private/render/renderer.cpp
    ./private/render/ui_pipeline.cpp
    ./private/control.cpp
    ./private/control_tree.cpp
    ./private/font.cpp
    ./private/ui.cpp)
add_library(violet::ui ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./public
    PRIVATE
    ./private)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    violet::common
    violet::core
    violet::graphics
    violet::window
    PRIVATE
    freetype
    violet::yoga)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)