#pragma once

#include "graphics/render_interface.hpp"
#include "math/math.hpp"

namespace violet
{
enum control_mesh_type
{
    ELEMENT_MESH_TYPE_BLOCK,
    ELEMENT_MESH_TYPE_TEXT,
    ELEMENT_MESH_TYPE_IMAGE
};

struct control_mesh
{
    control_mesh_type type;

    const float2* position;
    const float2* uv;
    const std::uint32_t* color;
    std::size_t vertex_count;

    const std::uint32_t* indices;
    std::size_t index_count;

    bool scissor;
    rhi_texture* texture;
};
} // namespace violet