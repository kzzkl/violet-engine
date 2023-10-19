#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
render_graph::render_graph(rhi_renderer* rhi) : m_context(rhi)
{
    m_context.add_parameter_layout(
        "violet mesh",
        {
            {RHI_PARAMETER_TYPE_UNIFORM_BUFFER, sizeof(float4x4)}
    });
    m_context.add_parameter_layout(
        "violet camera",
        {
            {RHI_PARAMETER_TYPE_UNIFORM_BUFFER, sizeof(float4x4) * 3}
    });
}

render_graph::~render_graph()
{
    m_material_layouts.clear();

    for (rhi_semaphore* sempahore : m_render_finished_semaphores)
        m_context.get_rhi()->destroy_semaphore(sempahore);
}

render_pass* render_graph::add_render_pass(std::string_view name)
{
    m_render_passes.push_back(std::make_unique<render_pass>(&m_context));
    return m_render_passes.back().get();
}

rhi_parameter_layout* render_graph::add_parameter_layout(
    std::string_view name,
    const std::vector<std::pair<rhi_parameter_type, std::size_t>>& layout)
{
    return m_context.add_parameter_layout(name, layout);
}

rhi_parameter_layout* render_graph::get_parameter_layout(std::string_view name) const
{
    return m_context.get_parameter_layout(name);
}

material_layout* render_graph::add_material_layout(std::string_view name)
{
    m_material_layouts[name.data()] = std::make_unique<material_layout>(&m_context);
    return m_material_layouts[name.data()].get();
}

material_layout* render_graph::get_material_layout(std::string_view name) const
{
    auto iter = m_material_layouts.find(name.data());
    if (iter != m_material_layouts.end())
        return iter->second.get();
    else
        return nullptr;
}

geometry* render_graph::add_geometry(std::string_view name)
{
    m_geometries[name.data()] = std::make_unique<geometry>(&m_context);
    return m_geometries[name.data()].get();
}

geometry* render_graph::get_geometry(std::string_view name) const
{
    auto iter = m_geometries.find(name.data());
    if (iter != m_geometries.end())
        return iter->second.get();
    else
        return nullptr;
}

bool render_graph::compile()
{
    for (auto& pass : m_render_passes)
    {
        if (!pass->compile())
            return false;
    }

    m_render_finished_semaphores.resize(m_context.get_rhi()->get_frame_resource_count());
    for (std::size_t i = 0; i < m_render_finished_semaphores.size(); ++i)
        m_render_finished_semaphores[i] = m_context.get_rhi()->create_semaphore();

    return true;
}

void render_graph::execute()
{
    rhi_renderer* rhi = m_context.get_rhi();
    rhi_render_command* command = rhi->allocate_command();

    for (auto& render_pass : m_render_passes)
        render_pass->execute(command);

    rhi_semaphore* signal_semaphore[] = {get_render_finished_semaphore()};
    rhi_semaphore* wait_semphores[] = {rhi->get_image_available_semaphore()};
    rhi->execute(&command, 1, signal_semaphore, 1, wait_semphores, 1, rhi->get_in_flight_fence());
}

rhi_semaphore* render_graph::get_render_finished_semaphore() const
{
    return m_render_finished_semaphores[m_context.get_rhi()->get_frame_resource_index()];
}
} // namespace violet