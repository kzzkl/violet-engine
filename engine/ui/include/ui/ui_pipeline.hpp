#pragma once

#include "graphics/render_pipeline.hpp"

namespace ash::ui
{
class ui_pipeline : public graphics::render_pipeline
{
public:
    ui_pipeline();

    virtual void render(
        const graphics::camera& camera,
        const graphics::render_scene& scene,
        graphics::render_command_interface* command) override;

private:
    std::unique_ptr<graphics::render_pipeline_interface> m_interface;
};
} // namespace ash::ui