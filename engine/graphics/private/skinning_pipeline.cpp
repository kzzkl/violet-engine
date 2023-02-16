#include "graphics/skinning_pipeline.hpp"

namespace violet::graphics
{
void skinning_pipeline::add(const skinned_mesh& skinned_mesh)
{
    skinning_item item = {};
    for (auto& skinned : skinned_mesh.skinned_vertex_buffers)
        item.skinned_vertex_buffers.push_back(skinned.get());
    item.parameter = skinned_mesh.parameter->interface();
    item.vertex_count = skinned_mesh.vertex_count;

    m_items.push_back(item);
}

void skinning_pipeline::skinning(render_command_interface* command)
{
    on_skinning(m_items, command);
    clear();
}
} // namespace violet::graphics