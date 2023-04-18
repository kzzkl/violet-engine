#pragma once

#include "graphics/rhi/pipeline_parameter.hpp"
#include "graphics/rhi/render_pipeline.hpp"

namespace violet
{
class basic_material : public pipeline_parameter
{
public:
    struct constant_data
    {
        float3 color;
    };

    static constexpr pipeline_parameter_desc layout = {
        .parameters = {{PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, sizeof(constant_data)}},
        .parameter_count = 1};

public:
    basic_material();

    void set_color(const float3& color);
    const float3& get_color() const noexcept;

private:
    constant_data m_data;
};

class basic_pipeline : public render_pipeline
{
protected:
    basic_pipeline();

    virtual void on_render(const std::vector<mesh*>& meshes, render_command_interface* command)
        override;

private:
    std::unique_ptr<render_pipeline_interface> m_interface;
};
} // namespace violet