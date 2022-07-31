#pragma once

#include "graphics/render_pipeline.hpp"

namespace ash::graphics
{
class shadow_pipeline : public render_pipeline
{
public:
    shadow_pipeline();

private:
    virtual void on_render(const render_scene& scene, render_command_interface* command) override;

    std::unique_ptr<render_pipeline_interface> m_interface;
};
} // namespace ash::graphics