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
    ui_pipeline(graphics::pipeline_layout* layout, graphics::pipeline* pipeline);

    virtual void render(
        graphics::resource* target,
        graphics::render_command* command,
        graphics::render_parameter* pass) override;

private:
};
} // namespace ash::ui