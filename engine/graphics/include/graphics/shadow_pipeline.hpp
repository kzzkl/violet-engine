#pragma once

#include "graphics/render_pipeline.hpp"

namespace ash::graphics
{
class shadow_pipeline : public render_pipeline
{
public:
    shadow_pipeline();

    virtual void render(const render_context& context, render_command_interface* command) override;

private:
    std::unique_ptr<render_pipeline_interface> m_interface;
};
} // namespace ash::graphics