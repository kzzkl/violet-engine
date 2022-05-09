#pragma once

#include "graphics.hpp"
#include "render_pipeline.hpp"

namespace ash::sample::mmd
{
class mmd_pass : public graphics::render_pass
{
public:
    mmd_pass();

    virtual void render(const graphics::camera& camera, graphics::render_command_interface* command)
        override;

private:
    std::unique_ptr<graphics::render_pass_interface> m_interface;
};
} // namespace ash::sample::mmd