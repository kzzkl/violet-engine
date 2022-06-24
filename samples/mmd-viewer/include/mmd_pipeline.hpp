#pragma once

#include "graphics/render_pipeline.hpp"
#include "graphics/skin_pipeline.hpp"
#include <array>

namespace ash::sample::mmd
{
class material_pipeline_parameter : public graphics::pipeline_parameter
{
public:
    material_pipeline_parameter();

    void diffuse(const math::float4& diffuse);
    void specular(const math::float3& specular);
    void specular_strength(float specular_strength);
    void toon_mode(std::uint32_t toon_mode);
    void spa_mode(std::uint32_t spa_mode);

    void tex(graphics::resource_interface* tex);
    void toon(graphics::resource_interface* toon);
    void spa(graphics::resource_interface* spa);

    static std::vector<graphics::pipeline_parameter_pair> layout();

private:
    struct constant_data
    {
        math::float4 diffuse;
        math::float3 specular;
        float specular_strength;
        std::uint32_t toon_mode;
        std::uint32_t spa_mode;
    };
};

class mmd_render_pipeline : public graphics::render_pipeline
{
public:
    mmd_render_pipeline();

    virtual void render(
        const graphics::camera& camera,
        const graphics::render_scene& scene,
        graphics::render_command_interface* command) override;

private:
    std::unique_ptr<graphics::render_pipeline_interface> m_interface;
};

class skin_pipeline_parameter : public graphics::pipeline_parameter
{
public:
    skin_pipeline_parameter();

    void bone_transform(const std::vector<math::float4x4>& bone_transform);
    void input_position(graphics::resource_interface* position);
    void input_normal(graphics::resource_interface* normal);
    void input_bone_index(graphics::resource_interface* bone_index);
    void input_bone_weight(graphics::resource_interface* bone_weight);
    void output_position(graphics::resource_interface* position);
    void output_normal(graphics::resource_interface* normal);

    static std::vector<graphics::pipeline_parameter_pair> layout();

private:
    struct constant_data
    {
        std::array<math::float4x4, 512> bone_transform;
    };
};

class mmd_skin_pipeline : public graphics::skin_pipeline
{
public:
    mmd_skin_pipeline();

    virtual void skin(graphics::render_command_interface* command) override;

private:
    std::unique_ptr<graphics::compute_pipeline_interface> m_interface;
};
} // namespace ash::sample::mmd