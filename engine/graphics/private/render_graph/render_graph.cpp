#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
render_graph::render_graph(rhi_context* rhi) : m_rhi(rhi)
{
}

render_graph::~render_graph()
{
    for (rhi_semaphore* sempahore : m_render_finished_semaphores)
        m_rhi->destroy_semaphore(sempahore);
}

render_pass* render_graph::add_render_pass(std::string_view name)
{
    m_render_passes.push_back(std::make_unique<render_pass>(name, m_rhi));
    return m_render_passes.back().get();
}

material_layout* render_graph::add_material_layout(std::string_view name)
{
    m_material_layouts[name.data()] = std::make_unique<material_layout>(name, m_rhi);
    return m_material_layouts[name.data()].get();
}

bool render_graph::compile()
{
    for (auto& pass : m_render_passes)
    {
        if (!pass->compile())
            return false;
    }

    m_render_finished_semaphores.resize(m_rhi->get_frame_resource_count());
    for (std::size_t i = 0; i < m_render_finished_semaphores.size(); ++i)
        m_render_finished_semaphores[i] = m_rhi->create_semaphore();

    return true;
}

void render_graph::execute()
{
    rhi_render_command* command = m_rhi->allocate_command();

    for (auto& render_pass : m_render_passes)
        render_pass->execute(command);

    rhi_semaphore* signal_semaphore[] = {get_render_finished_semaphore()};
    rhi_semaphore* wait_semphores[] = {m_rhi->get_image_available_semaphore()};
    m_rhi->execute(
        &command,
        1,
        signal_semaphore,
        1,
        wait_semphores,
        1,
        m_rhi->get_in_flight_fence());
}

rhi_semaphore* render_graph::get_render_finished_semaphore() const
{
    return m_render_finished_semaphores[m_rhi->get_frame_resource_index()];
}
} // namespace violet