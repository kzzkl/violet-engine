#pragma once

#include "component.hpp"
#include "graphics_interface.hpp"

namespace ash::graphics
{
struct mesh
{
    std::unique_ptr<resource> vertex_buffer;
    std::unique_ptr<resource> index_buffer;
};
} // namespace ash::graphics

namespace ash::ecs
{
template <>
struct component_trait<ash::graphics::mesh>
{
    static constexpr std::size_t id = ash::uuid("aafd80d3-465a-49c4-9d9b-21b02d069042").hash();
};
} // namespace ash::ecs