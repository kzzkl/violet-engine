#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
render_graph::render_graph(renderer* renderer) : m_renderer(renderer)
{
}

render_graph::~render_graph()
{
}

render_pass* render_graph::add_render_pass(std::string_view name)
{
    m_render_passes.push_back(std::make_unique<render_pass>(name, m_renderer));
    return m_render_passes.back().get();
}

render_pass* render_graph::get_render_pass(std::string_view name) const
{
    for (auto& render_pass : m_render_passes)
    {
        if (render_pass->get_name() == name)
            return render_pass.get();
    }
    return nullptr;
}

render_pipeline* render_graph::get_render_pipeline(std::string_view name) const
{
    for (auto& render_pass : m_render_passes)
    {
        render_pipeline* pipeline = render_pass->get_pipeline(name);
        if (pipeline != nullptr)
            return pipeline;
    }
    return nullptr;
}

material_layout* render_graph::add_material_layout(std::string_view name)
{
    m_material_layouts.push_back(std::make_unique<material_layout>(name, m_renderer));
    return m_material_layouts.back().get();
}

material_layout* render_graph::get_material_layout(std::string_view name) const
{
    for (auto& material_layout : m_material_layouts)
    {
        if (material_layout->get_name() == name)
            return material_layout.get();
    }
    return nullptr;
}

material* render_graph::add_material(std::string_view name, material_layout* layout)
{
    return layout->add_material(name);
}

material* render_graph::add_material(std::string_view name, std::string_view layout)
{
    return get_material_layout(layout)->add_material(name);
}

material* render_graph::get_material(std::string_view name, std::string_view layout) const
{
    return get_material_layout(layout)->get_material(name);
}

compute_pass* render_graph::add_compute_pass(std::string_view name)
{
    m_compute_passes.push_back(std::make_unique<compute_pass>(name, m_renderer));
    return m_compute_passes.back().get();
}

compute_pass* render_graph::get_compute_pass(std::string_view name) const
{
    for (auto& compute_pass : m_compute_passes)
    {
        if (compute_pass->get_name() == name)
            return compute_pass.get();
    }
    return nullptr;
}

compute_pipeline* render_graph::get_compute_pipeline(std::string_view name) const
{
    for (auto& compute_pass : m_compute_passes)
    {
        compute_pipeline* pipeline = compute_pass->get_pipeline(name);
        if (pipeline != nullptr)
            return pipeline;
    }
    return nullptr;
}

bool render_graph::compile()
{
    for (auto& pass : m_compute_passes)
    {
        if (!pass->compile())
            return false;
    }

    for (auto& pass : m_render_passes)
    {
        if (!pass->compile())
            return false;
    }

    m_render_finished_semaphores.resize(m_renderer->get_frame_resource_count());
    for (std::size_t i = 0; i < m_render_finished_semaphores.size(); ++i)
        m_render_finished_semaphores[i] = m_renderer->create_semaphore();

    return true;
}

void render_graph::execute(rhi_parameter* light)
{
    rhi_render_command* command = m_renderer->allocate_command();

    for (auto& compute_pass : m_compute_passes)
        compute_pass->execute(command);

    for (auto& render_pass : m_render_passes)
        render_pass->execute(command, light);

    m_renderer->execute(
        {command},
        {get_render_finished_semaphore()},
        {m_renderer->get_image_available_semaphore()},
        m_renderer->get_in_flight_fence());
}

rhi_semaphore* render_graph::get_render_finished_semaphore() const
{
    return m_render_finished_semaphores[m_renderer->get_frame_resource_index()].get();
}
} // namespace violet