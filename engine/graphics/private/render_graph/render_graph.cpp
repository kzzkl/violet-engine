#include "graphics/render_graph/render_graph.hpp"
#include <cassert>

namespace violet
{
render_graph::render_graph(renderer* renderer) : m_renderer(renderer)
{
    m_back_buffer = add_slot("back buffer input", RENDER_RESOURCE_TYPE_OUTPUT);
    m_back_buffer->set_format(renderer->get_back_buffer()->get_format());

    pass_slot* back_buffer_output = add_slot("back buffer output", RENDER_RESOURCE_TYPE_INPUT);
    back_buffer_output->set_input_layout(RHI_IMAGE_LAYOUT_PRESENT);
}

render_graph::~render_graph()
{
}

pass_slot* render_graph::add_slot(std::string_view name, pass_slot_type type)
{
    assert(m_slots.find(name.data()) == m_slots.end());

    m_slots[name.data()] = std::make_unique<pass_slot>(name, m_slots.size(), type);
    return m_slots[name.data()].get();
}

pass_slot* render_graph::get_slot(std::string_view name) const
{
    return m_slots.at(name.data()).get();
}

bool render_graph::compile()
{
    compile_context context = {};
    context.renderer = m_renderer;

    for (auto& [name, pass] : m_passes)
    {
        if (!pass->compile(context))
            return false;
        m_execute_list.push_back(pass.get());
    }

    m_render_finished_semaphores.resize(m_renderer->get_frame_resource_count());
    for (std::size_t i = 0; i < m_render_finished_semaphores.size(); ++i)
        m_render_finished_semaphores[i] = m_renderer->create_semaphore();

    return true;
}

void render_graph::execute()
{
    m_back_buffer->set_image(m_renderer->get_back_buffer());

    execute_context execute_context(m_renderer->allocate_command(), m_light, m_cameras);

    for (pass* pass : m_execute_list)
        pass->execute(execute_context);

    m_renderer->execute(
        {execute_context.get_command()},
        {get_render_finished_semaphore()},
        {m_renderer->get_image_available_semaphore()},
        m_renderer->get_in_flight_fence());

    for (auto& [name, pipeline] : m_material_pipelines)
        pipeline.first->clear_mesh();
}

rhi_semaphore* render_graph::get_render_finished_semaphore() const
{
    return m_render_finished_semaphores[m_renderer->get_frame_resource_index()].get();
}
} // namespace violet