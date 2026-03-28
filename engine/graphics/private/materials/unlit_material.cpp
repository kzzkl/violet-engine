#include "graphics/materials/unlit_material.hpp"
#include "graphics/shading_models/unlit_shading_model.hpp"

namespace violet
{
struct unlit_material_cs : public material_resolve_cs
{
    static constexpr std::string_view path = "assets/shaders/materials/unlit_material.hlsl";
};

unlit_material::unlit_material()
{
    set_cull_mode(RHI_CULL_MODE_BACK);
    set_polygon_mode(RHI_POLYGON_MODE_FILL);
    set_primitive_topology(RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    set_shading_model<unlit_shading_model>();
    set_surface_type(SURFACE_TYPE_OPAQUE);

    set_color({1.0f, 1.0f, 1.0f});
}

void unlit_material::set_color(const vec3f& color)
{
    get_constant().color = color;
}

rhi_shader* unlit_material::get_resolve_shader(std::span<std::wstring> defines) const
{
    return render_device::instance().get_shader<unlit_material_cs>(defines);
}
} // namespace violet