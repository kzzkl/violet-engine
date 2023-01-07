#pragma once

#include "graphics/render_pipeline.hpp"
#include "graphics/skinning_pipeline.hpp"
#include <array>

namespace violet::sample::mmd
{
class mmd_material_parameter : public graphics::pipeline_parameter
{
public:
    mmd_material_parameter();

    void diffuse(const math::float4& diffuse);
    void specular(const math::float3& specular);
    void specular_strength(float specular_strength);

    void edge_color(const math::float4& edge_color);
    void edge_size(float edge_size);

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
        math::float4 edge_color;
        math::float3 ambient;
        float edge_size;
        std::uint32_t toon_mode;
        std::uint32_t spa_mode;
    };

    constant_data m_data;
};

class mmd_render_pipeline : public graphics::render_pipeline
{
public:
    mmd_render_pipeline();

    virtual void render(
        const graphics::render_context& context,
        graphics::render_command_interface* command) override;

private:
    std::unique_ptr<graphics::render_pipeline_interface> m_interface;
};

class skinning_pipeline_parameter : public graphics::pipeline_parameter
{
public:
    skinning_pipeline_parameter();

    void bone_transform(const std::vector<math::float4x4>& bone_transform);
    void input_position(graphics::resource_interface* position);
    void input_normal(graphics::resource_interface* normal);
    void input_uv(graphics::resource_interface* uv);

    void skin(graphics::resource_interface* skin);
    void bdef_bone(graphics::resource_interface* bdef_bone);
    void sdef_bone(graphics::resource_interface* sdef_bone);

    void vertex_morph(graphics::resource_interface* vertex_morph);
    void uv_morph(graphics::resource_interface* uv_morph);

    void output_position(graphics::resource_interface* position);
    void output_normal(graphics::resource_interface* normal);
    void output_uv(graphics::resource_interface* uv);

    static std::vector<graphics::pipeline_parameter_pair> layout();

private:
    struct constant_data
    {
        std::array<math::float4x3, 512> bone_transform;
        std::array<math::float4, 512> bone_quaternion;
    };
};

class mmd_skinning_pipeline : public graphics::skinning_pipeline
{
public:
    mmd_skinning_pipeline();

private:
    virtual void on_skinning(
        const std::vector<graphics::skinning_item>& items,
        graphics::render_command_interface* command) override;

    std::unique_ptr<graphics::compute_pipeline_interface> m_interface;
};
} // namespace violet::sample::mmd