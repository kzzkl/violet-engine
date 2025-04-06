#include "graphics/materials/unlit_material.hpp"

namespace violet
{
struct unlit_material_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/materials/unlit_material.hlsl";
};

struct unlit_material_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/materials/unlit_material.hlsl";
};

unlit_material::unlit_material(
    rhi_primitive_topology primitive_topology,
    rhi_cull_mode cull_mode,
    rhi_polygon_mode polygon_mode)
    : mesh_material(MATERIAL_OPAQUE)
{
    auto& device = render_device::instance();

    auto& pipeline = get_pipeline();
    pipeline.vertex_shader = device.get_shader<unlit_material_vs>();
    pipeline.fragment_shader = device.get_shader<unlit_material_fs>();
    pipeline.rasterizer_state = device.get_rasterizer_state(cull_mode, polygon_mode);
    pipeline.depth_stencil_state = device.get_depth_stencil_state<
        true,
        true,
        RHI_COMPARE_OP_GREATER,
        true,
        material_stencil_state<SHADING_MODEL_UNLIT>::value,
        material_stencil_state<SHADING_MODEL_UNLIT>::value>();
    pipeline.primitive_topology = primitive_topology;

    set_color({1.0f, 1.0f, 1.0f});
}

void unlit_material::set_color(const vec3f& color)
{
    get_constant().color = color;
}
} // namespace violet