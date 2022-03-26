#pragma once

#include "assert.hpp"
#include "component.hpp"

namespace ash::graphics
{
struct submesh
{
    std::size_t index_start;
    std::size_t index_end;
};

class render_pipeline;
struct submesh_material
{
    render_pipeline* pipeline;
    std::unique_ptr<render_parameter> property;
};

struct visual
{
    std::unique_ptr<resource> vertex_buffer;
    std::unique_ptr<resource> index_buffer;

    std::vector<submesh> submesh;
    std::vector<submesh_material> material;

    std::unique_ptr<render_parameter> object;
};
} // namespace ash::graphics

namespace ash::ecs
{
template <>
struct component_trait<ash::graphics::visual>
{
    static constexpr std::size_t id = ash::uuid("be64e8c8-568e-4fdc-9d8c-d891c8ce2de0").hash();
};
} // namespace ash::ecs