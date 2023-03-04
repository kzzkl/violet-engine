#pragma once

#include "ui/color.hpp"
#include "ui/controls/dock_node.hpp"

namespace violet::ui
{
class panel;

struct dock_area_theme
{
    std::uint32_t hover_color;
};

class dock_area : public control
{
public:
    dock_area(int control_width, int control_height, const dock_area_theme& theme);

    void dock(dock_node* root);
    void dock(dock_node* source, dock_node* target, layout_edge edge);
    void undock(dock_node* control);

    void dock_begin(dock_node* control, int x, int y);
    void dock_move(int x, int y);
    void dock_end(int x, int y);

    bool resize() const noexcept { return m_resize_control != nullptr; }
    void resize(dock_node* control, layout_edge edge, int offset);
    void resize_end();

private:
    void move_up(dock_node* control);
    void move_down(dock_node* control);

    std::pair<dock_node*, layout_edge> find_mouse_over_control(int x, int y);
    dock_node* find_resize_control(dock_node* control, layout_edge edge);

    int m_area_width;
    int m_area_height;

    dock_node* m_docking_control;
    dock_node* m_resize_control;

    dock_node* m_hover_node;
    layout_edge m_hover_edge;
    std::unique_ptr<panel> m_hover_panel;
};
} // namespace violet::ui