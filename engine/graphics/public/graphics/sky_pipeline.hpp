#pragma once

#include "graphics/render_pipeline.hpp"

namespace violet::graphics
{
class sky_pipeline_parameter : public pipeline_parameter
{
public:
    static constexpr pipeline_parameter_desc layout = {
        .parameters = {{PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE, 1}},
        .parameter_count = 1};

public:
    sky_pipeline_parameter();

    void texture(resource_interface* texture);
};

class sky_pipeline : public render_pipeline
{
public:
    sky_pipeline();

    virtual void render(const render_context& context, render_command_interface* command) override;

private:
    std::unique_ptr<render_pipeline_interface> m_interface;
};
} // namespace violet::graphics