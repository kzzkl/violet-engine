#include "graphics/render_graph/render_graph.hpp"
#include <cassert>

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
    assert(m_render_pass_map.find(name.data()) == m_render_pass_map.end());

    m_render_passes.push_back(std::make_unique<render_pass>());
    m_render_pass_map[name.data()] = m_render_passes.back().get();
    return m_render_passes.back().get();
}

render_pass* render_graph::get_render_pass(std::string_view name) const
{
    return m_render_pass_map.at(name.data());
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

compute_pass* render_graph::add_compute_pass(std::string_view name)
{
    assert(m_compute_pass_map.find(name.data()) == m_compute_pass_map.end());

    m_compute_passes.push_back(std::make_unique<compute_pass>());
    m_compute_pass_map[name.data()] = m_compute_passes.back().get();
    return m_compute_passes.back().get();
}

compute_pass* render_graph::get_compute_pass(std::string_view name) const
{
    return m_compute_pass_map.at(name.data());
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

material_layout* render_graph::add_material_layout(std::string_view name)
{
    assert(m_material_layouts.find(name.data()) == m_material_layouts.end());
    m_material_layouts[name.data()] = std::make_unique<material_layout>();
    return m_material_layouts.at(name.data()).get();
}

material_layout* render_graph::get_material_layout(std::string_view name) const
{
    return m_material_layouts.at(name.data()).get();
}

material* render_graph::add_material(std::string_view name, material_layout* layout)
{
    return layout->add_material(name, m_renderer);
}

material* render_graph::add_material(std::string_view name, std::string_view layout)
{
    return get_material_layout(layout)->add_material(name, m_renderer);
}

material* render_graph::get_material(std::string_view name, std::string_view layout) const
{
    return get_material_layout(layout)->get_material(name);
}

bool render_graph::compile()
{
    compile_context context = {};
    context.renderer = m_renderer;

    for (auto& pass : m_compute_passes)
    {
        if (!pass->compile(context))
            return false;
    }

    for (auto& pass : m_render_passes)
    {
        if (!pass->compile(context))
            return false;
    }

    m_render_finished_semaphores.resize(m_renderer->get_frame_resource_count());
    for (std::size_t i = 0; i < m_render_finished_semaphores.size(); ++i)
        m_render_finished_semaphores[i] = m_renderer->create_semaphore();

    return true;
}

void render_graph::execute(rhi_parameter* light)
{
    execute_context context = {};
    context.command = m_renderer->allocate_command();
    context.light = light;

    for (auto& compute_pass : m_compute_passes)
        compute_pass->execute(context);

    for (auto& render_pass : m_render_passes)
        render_pass->execute(context);

    m_renderer->execute(
        {context.command},
        {get_render_finished_semaphore()},
        {m_renderer->get_image_available_semaphore()},
        m_renderer->get_in_flight_fence());
}

rhi_semaphore* render_graph::get_render_finished_semaphore() const
{
    return m_render_finished_semaphores[m_renderer->get_frame_resource_index()].get();
}
} // namespace violet