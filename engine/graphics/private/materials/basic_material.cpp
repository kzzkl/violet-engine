#include "graphics/materials/basic_material.hpp"

namespace violet
{
struct basic_material_vs : public vertex_shader
{
    static std::string_view get_path() { return "engine/shaders/basic.vs"; }

    static parameter_slots get_parameters()
    {
        return {
            {0, camera},
            {1, light },
            {2, mesh  }
        };
    };

    static input_slots get_inputs()
    {
        return {
            {"position", RHI_FORMAT_R32G32B32_FLOAT}
        };
    };
};

struct basic_material_fs : public fragment_shader
{
    static std::string_view get_path() { return "engine/shaders/basic.fs"; }

    static constexpr parameter material = {
        {RHI_PARAMETER_UNIFORM, RHI_SHADER_STAGE_FRAGMENT, sizeof(basic_material::data)}
    };

    static parameter_slots get_parameters()
    {
        return {
            {3, material}
        };
    };
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
    get_parameter(0)->set_uniform(0, &color, sizeof(float3), 0);
    m_data.color = color;
}

const float3& basic_material::get_color() const noexcept
{
    return m_data.color;
}
} // namespace violet