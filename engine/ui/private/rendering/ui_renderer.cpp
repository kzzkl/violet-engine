#include "rendering/ui_renderer.hpp"
#include "common/log.hpp"
#include <algorithm>

namespace violet
{
static constexpr std::size_t MAX_UI_VERTEX_COUNT = 4096 * 16;
static constexpr std::size_t MAX_UI_INDEX_COUNT = MAX_UI_VERTEX_COUNT * 2;

ui_renderer::ui_renderer(render_device* device)
    : m_painter_index(0),
      m_default_font(nullptr),
      m_device(device)
{
}

void ui_renderer::render(widget* root, ui_pass* pass)
{
    auto calculate_overlap_area = [](const widget_extent& a, const widget_extent& b)
    {
        float x1 = std::max(a.x, b.x);
        float x2 = std::min(a.x + a.width, b.x + b.width);
        float y1 = std::max(a.y, b.y);
        float y2 = std::min(a.y + a.height, b.y + b.height);
        return widget_extent{.x = x1, .y = y1, .width = x2 - x1, .height = y2 - y1};
    };

    widget_extent extent = root->get_extent();

    ui_painter* painter = allocate_painter();
    painter->push_group();
    painter->set_extent(extent.width, extent.height);

    render_widget(root, painter, extent);

    painter->compile();
    pass->add_painter(painter);
}

void ui_renderer::reset()
{
    for (std::size_t i = 0; i < m_painter_index; ++i)
        m_painter_pool[i]->reset();

    m_painter_index = 0;
}

void ui_renderer::set_default_font(font* font)
{
    m_default_font = font;
}

void ui_renderer::render_widget(widget* widget, ui_painter* painter, widget_extent visible_area)
{
    if (visible_area.width <= 0 || visible_area.height <= 0)
        return;

    widget->receive_event(widget_event::paint(painter));

    for (auto& child : widget->get_children())
    {
        if (!child->get_visible())
            continue;

        render_widget(child.get(), painter, visible_area);
    }
}

ui_painter* ui_renderer::allocate_painter()
{
    ++m_painter_index;

    if (m_painter_index > m_painter_pool.size())
        m_painter_pool.push_back(std::make_unique<ui_painter>(m_default_font, m_device));

    return m_painter_pool[m_painter_index - 1].get();
}
} // namespace violet