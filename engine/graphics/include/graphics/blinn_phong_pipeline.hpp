#pragma once

#include "graphics/render_pipeline.hpp"

namespace ash::graphics
{
class blinn_phong_material_pipeline_parameter : public pipeline_parameter
{
public:
    blinn_phong_material_pipeline_parameter();

    void diffuse(const math::float3& diffuse);
    void fresnel(const math::float3& fresnel);
    void roughness(float roughness);

    static std::vector<pipeline_parameter_pair> layout();

private:
    struct constant_data
    {
        math::float3 diffuse;
        float _padding_0;
        math::float3 fresnel;
        float roughness;
    };
};

class blinn_phong_pipeline : public render_pipeline
{
public:
    blinn_phong_pipeline();

private:
    virtual void on_render(const render_scene& scene, render_command_interface* command) override;

    std::unique_ptr<render_pipeline_interface> m_interface;
};
} // namespace ash::graphics