#include "cluster_material.hpp"

namespace violet
{
struct cluster_material_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/cluster_material.hlsl";
};

struct cluster_material_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/cluster_material.hlsl";
};

cluster_material::cluster_material()
    : mesh_material(MATERIAL_OPAQUE)
{
    auto& pipeline = get_pipeline();
    pipeline.vertex_shader = render_device::instance().get_shader<cluster_material_vs>();
    pipeline.fragment_shader = render_device::instance().get_shader<cluster_material_fs>();
    pipeline.primitive_topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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
}
} // namespace violet