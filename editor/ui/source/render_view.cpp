#include "render_view.hpp"

namespace ash::editor
{
void render_view::draw(ui::ui& ui, editor_data& data)
{
    ui.window("render");
    ui.texture(m_target.get());

    auto active = ui.any_item_active();
    if (m_resize_flag != active)
    {
        if (m_resize_flag)
        {
            auto [width, height] = ui.window_size();
            if (width != m_target_width || height != m_target_height)
            {
                m_target_width = width;
                m_target_height = height;
                resize_target();
            }
        }
        m_resize_flag = active;
    }

    ui.window_pop();
}

void render_view::resize_target()
{
    log::debug("resize: {} {}", m_target_width, m_target_height);
}
} // namespace ash::editor