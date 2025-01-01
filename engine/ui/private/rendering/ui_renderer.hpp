#pragma once

#include "ui/rendering/ui_painter.hpp"
#include "ui/rendering/ui_pass.hpp"
#include "ui/widget.hpp"
#include <memory>
#include <stack>
#include <unordered_map>

namespace violet
{
class ui_renderer
{
public:
    ui_renderer(render_device* device);

    void render(widget* root, ui_pass* pass);
    void reset();

    void set_default_font(font* font);

private:
    void render_widget(widget* widget, ui_painter* painter, widget_extent visible_area);

    ui_painter* allocate_painter();

    std::size_t m_painter_index;
    std::vector<std::unique_ptr<ui_painter>> m_painter_pool;

    font* m_default_font;

    render_device* m_device;
};
} // namespace violet