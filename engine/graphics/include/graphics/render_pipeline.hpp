#pragma once

#include "graphics/light.hpp"

namespace ash::graphics
{
struct render_unit
{
    resource_interface** vertex_buffers;
    resource_interface* index_buffer;

    std::size_t index_start;
    std::size_t index_end;
    std::size_t vertex_base;

    pipeline_parameter_interface** parameters;

    scissor_extent scissor;
};

struct render_scene
{
    pipeline_parameter_interface* camera_parameter;
    pipeline_parameter_interface* light_parameter;
    pipeline_parameter_interface* sky_parameter;

    resource_interface* render_target;
    resource_interface* render_target_resolve;
    resource_interface* depth_stencil_buffer;

    std::vector<render_unit> units;
};

class render_pipeline
{
public:
    render_pipeline();
    virtual ~render_pipeline() = default;

    virtual void render(const render_scene& scene, render_command_interface* command) = 0;
};
} // namespace ash::graphics