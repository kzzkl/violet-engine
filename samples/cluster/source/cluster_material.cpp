#include "cluster_material.hpp"

namespace violet
{
struct cluster_material_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/materials/cluster_material.hlsl";
};

struct cluster_material_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/materials/cluster_material.hlsl";
};

cluster_material::cluster_material()
    : mesh_material(MATERIAL_OPAQUE)
{
    auto& device = render_device::instance();

    auto& pipeline = get_pipeline();
    pipeline.vertex_shader = device.get_shader<cluster_material_vs>();
    pipeline.fragment_shader = device.get_shader<cluster_material_fs>();
    pipeline.depth_stencil_state = device.get_depth_stencil_state<
        true,
        true,
        RHI_COMPARE_OP_GREATER,
        true,
        material_stencil_state<SHADING_MODEL_UNLIT>::value,
        material_stencil_state<SHADING_MODEL_UNLIT>::value>();
    pipeline.primitive_topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}
} // namespace violet