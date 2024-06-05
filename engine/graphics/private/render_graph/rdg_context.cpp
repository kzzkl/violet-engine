#include "graphics/render_graph/rdg_context.hpp"
#include "graphics/render_graph/rdg_pass.hpp"

namespace violet
{
rdg_context::rdg_context(
    std::size_t resource_count,
    const std::vector<rdg_pass*>& passes,
    render_device* device)
    : m_resource_slots(resource_count),
      m_pass_slots(passes.size()),
      m_camera(nullptr),
      m_light(nullptr)
{
    for (rdg_pass* pass : passes)
    {
        pass->get_parameter_layout();
    }
}

void rdg_context::set_texture(std::size_t index, rhi_texture* texture)
{
    m_resource_slots[index].texture = texture;
}

rhi_texture* rdg_context::get_texture(std::size_t index)
{
    return m_resource_slots[index].texture;
}

rhi_texture* rdg_context::get_texture(rdg_pass* pass, std::size_t reference_index)
{
    return get_texture(pass->get_reference(reference_index)->resource->get_index());
}

void rdg_context::set_buffer(std::size_t index, rhi_buffer* buffer)
{
    m_resource_slots[index].buffer = buffer;
}

rhi_buffer* rdg_context::get_buffer(std::size_t index)
{
    return m_resource_slots[index].buffer;
}

void rdg_context::get_parameters(rdg_pass* pass)
{
}

void rdg_context::add_mesh(rdg_pass* pass, const rdg_mesh& mesh)
{
    m_pass_slots[pass->get_index()].meshes.push_back(mesh);
}

const std::vector<rdg_mesh>& rdg_context::get_meshes(rdg_pass* pass) const
{
    return m_pass_slots[pass->get_index()].meshes;
}

void rdg_context::add_dispatch(rdg_pass* pass, const rdg_dispatch& dispatch)
{
    m_pass_slots[pass->get_index()].dispatches.push_back(dispatch);
}

const std::vector<rdg_dispatch>& rdg_context::get_dispatches(rdg_pass* pass) const
{
    return m_pass_slots[pass->get_index()].dispatches;
}

void rdg_context::reset()
{
    for (auto& pass : m_pass_slots)
    {
        pass.meshes.clear();
        pass.dispatches.clear();
    }
}
} // namespace violet