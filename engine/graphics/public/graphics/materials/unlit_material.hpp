#pragma once

#include "graphics/material.hpp"

namespace violet
{
struct unlit_material_constant
{
    vec3f color;
};

class unlit_material : public mesh_material<unlit_material_constant, MATERIAL_PATH_VISIBILITY>
{
public:
    unlit_material(
        rhi_primitive_topology primitive_topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        rhi_cull_mode cull_mode = RHI_CULL_MODE_BACK,
        rhi_polygon_mode polygon_mode = RHI_POLYGON_MODE_FILL);

    void set_color(const vec3f& color);
};
} // namespace violet