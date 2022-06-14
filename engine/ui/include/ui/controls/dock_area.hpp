#pragma once

#include "ui/controls/dock_element.hpp"
#include "ui/controls/panel.hpp"

namespace ash::ui
{
class dock_area : public element
{
public:
    dock_area(int element_width, int element_height, std::uint32_t hover_color = COLOR_WHITE);

    void dock(dock_element* root);
    void dock(dock_element* source, dock_element* target, layout_edge edge);
    void undock(dock_element* element);

    void dock_begin(dock_element* element, int x, int y);
    void dock_move(int x, int y);
    void dock_end(int x, int y);

    bool resize() const noexcept { return m_resize_element != nullptr; }
    void resize(dock_element* element, layout_edge edge, int offset);
    void resize_end();

private:
    void move_up(dock_element* element);
    void move_down(dock_element* element);

    std::pair<dock_element*, layout_edge> find_mouse_over_element(int x, int y);
    dock_element* find_resize_element(dock_element* element, layout_edge edge);

    int m_area_width;
    int m_area_height;

    dock_element* m_docking_element;
    dock_element* m_resize_element;

    dock_element* m_hover_node;
    layout_edge m_hover_edge;
    std::unique_ptr<panel> m_hover_panel;
};
} // namespace ash::ui