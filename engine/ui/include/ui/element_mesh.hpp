#pragma once

#include "graphics_interface.hpp"

namespace violet::ui
{
enum element_mesh_type
{
    ELEMENT_MESH_TYPE_BLOCK,
    ELEMENT_MESH_TYPE_TEXT,
    ELEMENT_MESH_TYPE_IMAGE
};

struct element_mesh
{
    element_mesh_type type;

    const math::float2* position;
    const math::float2* uv;
    const std::uint32_t* color;
    std::size_t vertex_count;

    const std::uint32_t* indices;
    std::size_t index_count;

    bool scissor;
    graphics::resource_interface* texture;
};
} // namespace violet::ui