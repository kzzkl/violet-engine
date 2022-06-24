#pragma once

#include "graphics/camera.hpp"
#include "graphics/light.hpp"
#include "graphics/mesh_render.hpp"

namespace ash::graphics
{
struct render_unit
{
    std::vector<resource_interface*> vertex_buffers;
    resource_interface* index_buffer;

    std::size_t index_start;
    std::size_t index_end;
    std::size_t vertex_base;
    std::vector<pipeline_parameter_interface*> parameters;

    scissor_extent scissor;
};

struct render_scene
{
    pipeline_parameter_interface* light_parameter;
    std::vector<render_unit> units;
};

class render_pipeline
{
public:
    render_pipeline();
    virtual ~render_pipeline() = default;

    virtual void render(
        const camera& camera,
        const render_scene& scene,
        render_command_interface* command) = 0;
};
} // namespace ash::graphics