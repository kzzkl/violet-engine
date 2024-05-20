#include "graphics/render_graph/rdg_context.hpp"
#include "graphics/render_graph/rdg_pass.hpp"

namespace violet
{
rdg_context::rdg_context(std::size_t resource_count, std::size_t pass_count)
    : m_resource_slots(resource_count),
      m_camera(nullptr),
      m_light(nullptr),
      m_mesh_queues(pass_count),
      m_dispatch_queues(pass_count)
{
}

void rdg_context::set_texture(std::size_t index, rhi_texture* texture)
{
    m_resource_slots[index].texture = texture;
}

rhi_texture* rdg_context::get_texture(std::size_t index)
{
    return m_resource_slots[index].texture;
}

void rdg_context::set_buffer(std::size_t index, rhi_buffer* buffer)
{
    m_resource_slots[index].buffer = buffer;
}

rhi_buffer* rdg_context::get_buffer(std::size_t index)
{
    return m_resource_slots[index].buffer;
}

void rdg_context::add_mesh(rdg_pass* pass, const rdg_mesh& mesh)
{
    m_mesh_queues[pass->get_index()].push_back(mesh);
}

const std::vector<rdg_mesh>& rdg_context::get_meshes(rdg_pass* pass) const
{
    return m_mesh_queues[pass->get_index()];
}

void rdg_context::add_dispatch(rdg_pass* pass, const rdg_dispatch& dispatch)
{
    m_dispatch_queues[pass->get_index()].push_back(dispatch);
}

const std::vector<rdg_dispatch>& rdg_context::get_dispatches(rdg_pass* pass) const
{
    return m_dispatch_queues[pass->get_index()];
}

void rdg_context::reset()
{
    for (auto& queue : m_mesh_queues)
        queue.clear();

    for (auto& queue : m_dispatch_queues)
        queue.clear();
}
} // namespace violet