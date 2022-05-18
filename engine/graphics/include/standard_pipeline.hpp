#pragma once

#include "render_pipeline.hpp"

namespace ash::graphics
{
class standard_pass : public render_pass
{
public:
    standard_pass();

    virtual void render(const camera& camera, render_command_interface* command) override;

private:
    std::unique_ptr<render_pass_interface> m_interface;
};
} // namespace ash::graphics