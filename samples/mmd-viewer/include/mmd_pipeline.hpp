#pragma once

#include "graphics/graphics.hpp"
#include "render_pipeline.hpp"
#include "skin_pipeline.hpp"

namespace ash::sample::mmd
{
class mmd_render_pipeline : public graphics::render_pipeline
{
public:
    mmd_render_pipeline();

    virtual void render(const graphics::camera& camera, graphics::render_command_interface* command)
        override;

private:
    std::unique_ptr<graphics::render_pass_interface> m_interface;
};

class mmd_skin_pipeline : public graphics::skin_pipeline
{
public:
    mmd_skin_pipeline();

    virtual void skin(graphics::render_command_interface* command) override;

private:
    std::unique_ptr<graphics::compute_pipeline_interface> m_interface;
};
} // namespace ash::sample::mmd