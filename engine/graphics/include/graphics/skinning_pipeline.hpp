#pragma once

#include "graphics/mesh_render.hpp"
#include "graphics/skinned_mesh.hpp"

namespace ash::graphics
{
struct skinning_item
{
    std::vector<resource_interface*> input_vertex_buffers;
    std::vector<resource_interface*> skinned_vertex_buffers;
    pipeline_parameter_interface* parameter;

    std::size_t vertex_count;
};

class skinning_pipeline
{
public:
    virtual ~skinning_pipeline() = default;

    void add(const skinned_mesh& skinned_mesh);
    void clear() noexcept { m_items.clear(); }

    void skinning(render_command_interface* command);

private:
    virtual void on_skinning(render_command_interface* command) = 0;

    std::vector<skinning_item> m_items;
};
} // namespace ash::graphics