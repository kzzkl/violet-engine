#pragma once

#include "ui/layout/layout.hpp"
#include "ui/rendering/ui_draw_list.hpp"
#include <memory>
#include <vector>

namespace violet
{
using widget_extent = rect<std::uint32_t>;

class widget
{
public:
    widget();
    virtual ~widget();

    void add(std::shared_ptr<widget> child, std::size_t index = -1);
    void remove(std::shared_ptr<widget> child);

    void set_visible(bool visible) noexcept { m_visible = visible; }
    bool get_visible() const noexcept { return m_visible; }

    widget_extent get_extent() const;

    layout_node* get_layout() const noexcept { return m_layout.get(); }

    const std::vector<std::shared_ptr<widget>>& get_children() const noexcept { return m_children; }

    void paint(ui_draw_list* draw_list);

private:
    virtual void on_paint(ui_draw_list* draw_list) {}

    bool m_visible;

    std::size_t m_index;
    std::size_t m_layer;

    std::vector<std::shared_ptr<widget>> m_children;

    std::unique_ptr<layout_node> m_layout;
};
} // namespace violet