#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
render_graph::render_graph(rhi_context* rhi) : m_rhi(rhi)
{
    render_resource* back_buffer = add_resource("back buffer");
    back_buffer->set_format(m_rhi->get_back_buffer()->get_format());
    back_buffer->set_resource(m_rhi->get_back_buffer());
}

render_graph::~render_graph()
{
}

render_resource* render_graph::add_resource(std::string_view name)
{
    m_resources.push_back(std::make_unique<render_resource>(name, m_rhi));
    return m_resources.back().get();
}

render_pass* render_graph::add_render_pass(std::string_view name)
{
    m_render_passes.push_back(std::make_unique<render_pass>(name, m_rhi));
    return m_render_passes.back().get();
}

bool render_graph::compile()
{
    for (auto& pass : m_render_passes)
    {
        if (!pass->compile())
            return false;
    }

    for (auto& pass : m_render_passes)
    {
        if (!pass->compile())
            return false;
    }

    return true;
}

void render_graph::execute()
{
    get_back_buffer()->set_resource(m_rhi->get_back_buffer());

    rhi_render_command* command = m_rhi->allocate_command();

    rhi_scissor_rect rect = {
        .min_x = 0,
        .min_y = 0,
        .max_x = m_rhi->get_back_buffer()->get_extent().width,
        .max_y = m_rhi->get_back_buffer()->get_extent().height};
    command->set_scissor(&rect, 1);

    for (auto& render_pass : m_render_passes)
        render_pass->execute(command);

    m_rhi->execute(command, m_rhi->get_fence());
}

render_resource* render_graph::get_back_buffer()
{
    return m_resources[0].get();
}
} // namespace violet