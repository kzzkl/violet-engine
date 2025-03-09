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

unlit_material::unlit_material()
    : mesh_material(MATERIAL_OPAQUE)
{
    auto& pipeline = get_pipeline();
    pipeline.vertex_shader = render_device::instance().get_shader<unlit_material_vs>();
    pipeline.fragment_shader = render_device::instance().get_shader<unlit_material_fs>();
    pipeline.depth_stencil.depth_enable = true;
    pipeline.depth_stencil.depth_write_enable = true;
    pipeline.depth_stencil.depth_compare_op = RHI_COMPARE_OP_GREATER;
    pipeline.depth_stencil.stencil_enable = true;
    pipeline.depth_stencil.stencil_front = {
        .compare_op = RHI_COMPARE_OP_ALWAYS,
        .pass_op = RHI_STENCIL_OP_REPLACE,
        .depth_fail_op = RHI_STENCIL_OP_KEEP,
        .reference = SHADING_MODEL_UNLIT,
    };
    pipeline.depth_stencil.stencil_back = pipeline.depth_stencil.stencil_front;

    set_color({1.0f, 1.0f, 1.0f});
}

void unlit_material::set_color(const vec3f& color)
{
    get_constant().color = color;
}

unlit_line_material::unlit_line_material()
    : mesh_material(MATERIAL_OPAQUE)
{
    auto& pipeline = get_pipeline();
    pipeline.vertex_shader = render_device::instance().get_shader<unlit_material_vs>();
    pipeline.fragment_shader = render_device::instance().get_shader<unlit_material_fs>();
    pipeline.primitive_topology = RHI_PRIMITIVE_TOPOLOGY_LINE_LIST;
    pipeline.depth_stencil.depth_enable = true;
    pipeline.depth_stencil.depth_write_enable = true;
    pipeline.depth_stencil.depth_compare_op = RHI_COMPARE_OP_GREATER;
    pipeline.depth_stencil.stencil_enable = true;
    pipeline.depth_stencil.stencil_front = {
        .compare_op = RHI_COMPARE_OP_ALWAYS,
        .pass_op = RHI_STENCIL_OP_REPLACE,
        .depth_fail_op = RHI_STENCIL_OP_KEEP,
        .reference = SHADING_MODEL_UNLIT,
    };
    pipeline.depth_stencil.stencil_back = pipeline.depth_stencil.stencil_front;

    set_color({1.0f, 1.0f, 1.0f});
}

void unlit_line_material::set_color(const vec3f& color)
{
    get_constant().color = color;
}
} // namespace violet