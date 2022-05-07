#pragma once

#include "graphics.hpp"
#include "render_pipeline.hpp"

namespace ash::sample::mmd
{
class mmd_pass : public graphics::render_pass
{
public:
    mmd_pass(graphics::graphics& graphics);
    virtual void render(const graphics::camera& camera, graphics::render_command_interface* command)
        override;

private:
    void initialize_interface(graphics::graphics& graphics);
    void initialize_attachment_set(graphics::graphics& graphics);

    std::vector<std::unique_ptr<graphics::attachment_set_interface>> m_attachment_sets;

    std::unique_ptr<graphics::resource> m_render_target;
    std::unique_ptr<graphics::resource> m_depth_stencil;

    std::unique_ptr<graphics::render_pass_interface> m_interface;
};
} // namespace ash::sample::mmd