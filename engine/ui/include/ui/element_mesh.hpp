#pragma once

#include "graphics_interface.hpp"
#include <vector>

namespace ash::ui
{
struct element_mesh
{
    void reset()
    {
        vertex_position.clear();
        vertex_uv.clear();
        vertex_color.clear();
        indices.clear();

        texture = nullptr;
    }

    std::vector<math::float3> vertex_position;
    std::vector<math::float2> vertex_uv;
    std::vector<std::uint32_t> vertex_color;
    std::vector<std::uint32_t> indices;

    graphics::resource* texture;
};
} // namespace ash::ui