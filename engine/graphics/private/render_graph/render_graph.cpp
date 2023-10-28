#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
render_graph::render_graph(graphics_context* context) : m_context(context)
{
}

render_graph::~render_graph()
{
    m_material_layouts.clear();

    for (rhi_semaphore* sempahore : m_render_finished_semaphores)
        m_context->get_rhi()->destroy_semaphore(sempahore);
}

render_pass* render_graph::add_render_pass(std::string_view name)
{
    m_render_passes.push_back(std::make_unique<render_pass>(m_context));
    return m_render_passes.back().get();
}

material_layout* render_graph::add_material_layout(std::string_view name)
{
    m_material_layouts[name.data()] = std::make_unique<material_layout>(m_context);
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

bool render_graph::compile()
{
    for (auto& pass : m_render_passes)
    {
        if (!pass->compile())
            return false;
    }

    m_render_finished_semaphores.resize(m_context->get_rhi()->get_frame_resource_count());
    for (std::size_t i = 0; i < m_render_finished_semaphores.size(); ++i)
        m_render_finished_semaphores[i] = m_context->get_rhi()->create_semaphore();

    return true;
}

void render_graph::execute()
{
    rhi_renderer* rhi = m_context->get_rhi();
    rhi_render_command* command = rhi->allocate_command();

    for (auto& render_pass : m_render_passes)
        render_pass->execute(command);

    rhi_semaphore* signal_semaphore[] = {get_render_finished_semaphore()};
    rhi_semaphore* wait_semphores[] = {rhi->get_image_available_semaphore()};
    rhi->execute(&command, 1, signal_semaphore, 1, wait_semphores, 1, rhi->get_in_flight_fence());
}

rhi_semaphore* render_graph::get_render_finished_semaphore() const
{
    return m_render_finished_semaphores[m_context->get_rhi()->get_frame_resource_index()];
}
} // namespace violet