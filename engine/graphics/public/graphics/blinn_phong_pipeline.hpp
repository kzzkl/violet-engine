#pragma once

#include "graphics/render_pipeline.hpp"

namespace violet::graphics
{
class blinn_phong_material_pipeline_parameter : public pipeline_parameter
{
public:
    struct constant_data
    {
        math::float3 diffuse;
        float _padding_0;
        math::float3 fresnel;
        float roughness;
    };

    static constexpr pipeline_parameter_desc layout = {
        .parameters = {{PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, sizeof(constant_data)}},
        .parameter_count = 1};

public:
    blinn_phong_material_pipeline_parameter();

    void diffuse(const math::float3& diffuse);
    void fresnel(const math::float3& fresnel);
    void roughness(float roughness);
};

class blinn_phong_pipeline : public render_pipeline
{
public:
    blinn_phong_pipeline();

    virtual void render(const render_context& context, render_command_interface* command) override;

private:
    std::unique_ptr<render_pipeline_interface> m_interface;
};
} // namespace violet::graphics