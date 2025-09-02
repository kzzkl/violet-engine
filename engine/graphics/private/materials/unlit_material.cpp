#include "graphics/materials/unlit_material.hpp"
#include "graphics/shading_models/unlit_shading_model.hpp"

namespace violet
{
struct unlit_material_cs : public material_resolve_cs
{
    static constexpr std::string_view path = "assets/shaders/materials/unlit_material.hlsl";
};

unlit_material::unlit_material(
    rhi_primitive_topology primitive_topology,
    rhi_cull_mode cull_mode,
    rhi_polygon_mode polygon_mode)
{
    auto& device = render_device::instance();

    set_pipeline<unlit_shading_model>(
        {
            .vertex_shader = device.get_shader<visibility_vs>(),
            .fragment_shader = device.get_shader<visibility_fs>(),
            .rasterizer_state = device.get_rasterizer_state(cull_mode, polygon_mode),
            .depth_stencil_state =
                device.get_depth_stencil_state<true, true, RHI_COMPARE_OP_GREATER>(),
            .primitive_topology = primitive_topology,
        },
        {
            .compute_shader = device.get_shader<unlit_material_cs>(),
        });
    set_surface_type(SURFACE_TYPE_OPAQUE);

    set_color({1.0f, 1.0f, 1.0f});
}

void unlit_material::set_color(const vec3f& color)
{
    get_constant().color = color;
}
} // namespace violet