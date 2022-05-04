#pragma once

#include "graphics.hpp"
#include "render_pipeline.hpp"

namespace ash::sample::mmd
{
class mmd_pass : public graphics::technique
{
public:
    mmd_pass(graphics::technique_interface* interface);
    virtual void render(const graphics::camera& camera, graphics::render_command_interface* command)
        override;

    void initialize_render_target_set(graphics::graphics& graphics);

private:
    std::vector<std::unique_ptr<graphics::render_target_set_interface>> m_render_target_sets;
    std::unique_ptr<graphics::resource> m_depth_stencil;
};
} // namespace ash::sample::mmd