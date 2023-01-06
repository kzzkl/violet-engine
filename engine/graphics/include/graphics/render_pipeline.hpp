#pragma once

#include "graphics/light.hpp"
#include <map>

namespace ash::graphics
{
struct render_item
{
    resource_interface** vertex_buffers;
    resource_interface* index_buffer;

    std::size_t index_start;
    std::size_t index_end;
    std::size_t vertex_base;

    pipeline_parameter_interface* object_parameter;
    pipeline_parameter_interface* material_parameter;

    scissor_extent scissor;
};

class shadow_map;
struct render_context
{
    pipeline_parameter_interface* camera_parameter;
    pipeline_parameter_interface* light_parameter;
    pipeline_parameter_interface* sky_parameter;

    shadow_map* shadow_map;

    resource_interface* render_target;
    resource_interface* render_target_resolve;
    resource_interface* depth_stencil_buffer;

    std::vector<render_item> items;
};

class render_pipeline
{
public:
    render_pipeline();
    virtual ~render_pipeline() = default;

    virtual void render(const render_context& context, render_command_interface* command) = 0;
};
} // namespace ash::graphics