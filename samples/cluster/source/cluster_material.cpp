#include "cluster_material.hpp"
#include "graphics/shading_models/pbr_shading_model.hpp"

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
{
    auto& device = render_device::instance();

    set_pipeline<pbr_shading_model>({
        .vertex_shader = device.get_shader<cluster_material_vs>(),
        .fragment_shader = device.get_shader<cluster_material_fs>(),
        .depth_stencil_state = device.get_depth_stencil_state<true, true, RHI_COMPARE_OP_GREATER>(),
    });
    set_surface_type(SURFACE_TYPE_OPAQUE);
}
} // namespace violet