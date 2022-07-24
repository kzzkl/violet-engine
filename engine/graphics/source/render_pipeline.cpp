#include "graphics/render_pipeline.hpp"

namespace ash::graphics
{
render_pipeline::render_pipeline() : m_scene{}
{
}

void render_pipeline::camera_parameter(pipeline_parameter_interface* parameter) noexcept
{
    m_scene.camera_parameter = parameter;
}

void render_pipeline::light_parameter(pipeline_parameter_interface* parameter) noexcept
{
    m_scene.light_parameter = parameter;
}

void render_pipeline::sky_parameter(pipeline_parameter_interface* parameter) noexcept
{
    m_scene.sky_parameter = parameter;
}

void render_pipeline::shadow(shadow_map* shadow_map) noexcept
{
    m_scene.shadow_map = shadow_map;
}

void render_pipeline::render_target(
    resource_interface* render_target,
    resource_interface* render_target_resolve,
    resource_interface* depth_stencil_buffer)
{
    m_scene.render_target = render_target;
    m_scene.render_target_resolve = render_target_resolve;
    m_scene.depth_stencil_buffer = depth_stencil_buffer;
}

void render_pipeline::add_item(const render_item& item)
{
    m_scene.items.emplace_back(item);
}

void render_pipeline::clear()
{
    m_scene.camera_parameter = nullptr;
    m_scene.light_parameter = nullptr;
    m_scene.sky_parameter = nullptr;

    m_scene.shadow_map = nullptr;

    m_scene.render_target = nullptr;
    m_scene.render_target_resolve = nullptr;
    m_scene.depth_stencil_buffer = nullptr;

    m_scene.items.clear();
}

void render_pipeline::render(render_command_interface* command)
{
    on_render(m_scene, command);
    clear();
}
} // namespace ash::graphics