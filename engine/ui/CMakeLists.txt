project(violet-ui)

# Build yoga layout engine.
# file(GLOB_RECURSE VIOLET_YOGA_SOURCE ${VIOLET_THIRD_PARTY_DIR}/yoga/yoga/*.cpp)
# add_library(violet-yoga STATIC ${VIOLET_YOGA_SOURCE})
# target_include_directories(violet-yoga PUBLIC ${VIOLET_THIRD_PARTY_DIR}/yoga/.)
# add_library(violet::yoga ALIAS violet-yoga)

# Build ui system.
add_library(${PROJECT_NAME} STATIC
    ./private/components/ui_root.cpp
    ./private/widgets/button.cpp
    # ./private/widgets/collapse.cpp
    # ./private/widgets/dock_area.cpp
    # ./private/widgets/dock_node.cpp
    # ./private/widgets/dock_window.cpp
    # ./private/widgets/font_icon.cpp
    # ./private/widgets/image.cpp
    # ./private/widgets/input.cpp
    ./private/widgets/label.cpp
    ./private/widgets/node_editor.cpp
    ./private/widgets/panel.cpp
    # ./private/widgets/scroll_view.cpp
    # ./private/widgets/tree.cpp
    ./private/event/event.cpp
    ./private/layout/layout_yoga.cpp
    ./private/rendering/ui_painter.cpp
    ./private/rendering/ui_pass.cpp
    ./private/rendering/ui_renderer.cpp
    # ./private/control.cpp
    # ./private/control_tree.cpp
    ./private/font.cpp
    ./private/ui_module.cpp
    ./private/widget.cpp)
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
    yogacore)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)