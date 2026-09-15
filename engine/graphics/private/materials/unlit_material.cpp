#include "graphics/materials/unlit_material.hpp"
#include "graphics/shading_models/unlit_shading_model.hpp"

namespace violet
{
unlit_material::unlit_material()
    : material_instance("unlit_material")
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
} // namespace violet