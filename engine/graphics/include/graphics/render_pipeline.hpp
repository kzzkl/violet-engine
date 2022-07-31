#pragma once

#include "graphics/light.hpp"

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
    pipeline_parameter_interface** additional_parameters;

    scissor_extent scissor;
};

class shadow_map;
struct render_scene
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

    void camera_parameter(pipeline_parameter_interface* parameter) noexcept;
    void light_parameter(pipeline_parameter_interface* parameter) noexcept;
    void sky_parameter(pipeline_parameter_interface* parameter) noexcept;
    void shadow(shadow_map* shadow_map) noexcept;

    void render_target(
        resource_interface* render_target,
        resource_interface* render_target_resolve,
        resource_interface* depth_stencil_buffer);

    void add_item(const render_item& item);
    void clear();

    void render(render_command_interface* command);

private:
    virtual void on_render(const render_scene& scene, render_command_interface* command) = 0;

    render_scene m_scene;
};
} // namespace ash::graphics