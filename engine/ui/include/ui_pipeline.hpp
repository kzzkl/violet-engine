#pragma once

#include "render_pipeline.hpp"

namespace ash::ui
{
struct ui_render_data
{
    std::size_t clip_min_x;
    std::size_t clip_max_x;
    std::size_t clip_min_y;
    std::size_t clip_max_y;
};

class ui_pipeline : public graphics::render_pipeline
{
public:
    ui_pipeline();

    virtual void render(const graphics::camera& camera, graphics::render_command_interface* command)
        override;

    void add_scissor(const graphics::scissor_rect& rect) { m_scissor_rects.push_back(rect); }

private:
    std::unique_ptr<graphics::render_pass_interface> m_interface;
    std::vector<graphics::scissor_rect> m_scissor_rects;
};
} // namespace ash::ui