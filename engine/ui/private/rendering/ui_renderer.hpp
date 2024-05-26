#pragma once

#include "rendering/ui_render_graph.hpp"
#include "ui/rendering/ui_draw_list.hpp"
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

private:
    void render_widget(widget* widget, ui_draw_list* draw_list, widget_extent visible_area);

    ui_draw_list* allocate_list();

    std::vector<float4> m_offset; // x, y, depth

    std::size_t m_draw_list_index;
    std::vector<std::unique_ptr<ui_draw_list>> m_draw_list_pool;

    std::size_t m_material_parameter_counter;

    render_device* m_device;
};
} // namespace violet