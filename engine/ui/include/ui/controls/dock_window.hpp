#pragma once

#include "ui/color.hpp"
#include "ui/controls/font_icon.hpp"
#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"
#include "ui/element.hpp"

namespace ash::ui
{
class dock_intermediate_node;
class dock_element : public element
{
public:
    dock_element();

    void dock(dock_element* target, layout_edge edge);
    void undock();

    virtual bool dockable() const noexcept { return true; }

    float m_width;
    float m_height;

protected:
    bool is_root() const noexcept { return m_dock_node == nullptr; }
    dock_intermediate_node* dock_node() const noexcept { return m_dock_node.get(); }

    std::shared_ptr<dock_intermediate_node> m_dock_node;

private:
    void move_up();
    void move_down();

    void resize_percent(float width, float height);
};

class dock_panel : public dock_element
{
public:
    dock_panel(std::uint32_t color = COLOR_WHITE);

    void color(std::uint32_t color) noexcept;

    virtual void render(renderer& renderer) override;

protected:
    virtual void on_extent_change() override;
};

struct dock_window_style
{
    const font* icon_font{nullptr};
    std::uint32_t icon_color{COLOR_BLACK};
    float icon_scale{0.8f};

    const font* title_font{nullptr};
    std::uint32_t title_color{COLOR_BLACK};

    std::uint32_t bar_color{COLOR_WHITE};
    std::uint32_t container_color{COLOR_WHITE};
};

class dock_window : public dock_panel
{
public:
    dock_window(std::string_view title, const dock_window_style& style);
    dock_window(std::string_view title, std::uint32_t icon_index, const dock_window_style& style);

private:
    layout_edge mouse_over_edge(int mouse_x, int mouse_y) const noexcept;

    std::unique_ptr<element> m_bar;
    std::unique_ptr<font_icon> m_icon;
    std::unique_ptr<label> m_title;

    std::unique_ptr<panel> m_container;

    layout_edge m_drag_edge;
    int m_original_position;
};
} // namespace ash::ui