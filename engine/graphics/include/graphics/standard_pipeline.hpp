#pragma once

#include "graphics/render_pipeline.hpp"

namespace ash::graphics
{
class standard_material_pipeline_parameter : public pipeline_parameter
{
public:
    standard_material_pipeline_parameter();

    void diffuse(const math::float3& diffuse);
    static std::vector<pipeline_parameter_pair> layout();

private:
    struct constant_data
    {
        math::float3 diffuse;
        float _padding_1;
    };
};

class standard_pipeline : public render_pipeline
{
public:
    standard_pipeline();

    virtual void render(
        const camera& camera,
        const render_scene& scene,
        render_command_interface* command) override;

private:
    std::unique_ptr<render_pipeline_interface> m_interface;
};
} // namespace ash::graphics