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

class ui_pass : public graphics::render_pass
{
public:
    ui_pass(graphics::technique_interface* interface);

    virtual void render(const graphics::camera& camera, graphics::render_command_interface* command)
        override;

private:
};
} // namespace ash::ui