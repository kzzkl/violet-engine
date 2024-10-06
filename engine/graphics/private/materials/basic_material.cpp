#include "graphics/materials/basic_material.hpp"

namespace violet
{
struct basic_material_vs : public shader_vs
{
    static constexpr std::string_view path = "assets/shaders/source/materials/basic.hlsl";

    static constexpr parameter_layout parameters = {{0, camera}, {1, light}, {2, mesh}};

    static constexpr input_layout inputs = {{"position", RHI_FORMAT_R32G32B32_FLOAT}};
};

struct basic_material_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/source/materials/basic.hlsl";

    static constexpr parameter material = {
        {RHI_PARAMETER_UNIFORM, RHI_SHADER_STAGE_FRAGMENT, sizeof(basic_material::data)}};

    static constexpr parameter_layout parameters = {{3, material}};
};

basic_material::basic_material(const float3& color)
{
    rdg_render_pipeline pipeline = {};
    pipeline.vertex_shader = render_device::instance().get_shader<basic_material_vs>();
    pipeline.fragment_shader = render_device::instance().get_shader<basic_material_fs>();

    add_pass(pipeline, basic_material_fs::material);

    set_color(color);
}

void basic_material::set_color(const float3& color)
{
    get_passes()[0].parameter->set_uniform(0, &color, sizeof(float3));
    m_data.color = color;
}

const float3& basic_material::get_color() const noexcept
{
    return m_data.color;
}
} // namespace violet