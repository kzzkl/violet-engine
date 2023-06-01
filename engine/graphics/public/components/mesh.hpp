#pragma once

#include "graphics/render_graph/node_parameter.hpp"
#include "graphics/render_graph/render_pipeline.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace violet
{
struct submesh
{
    std::size_t index_offset;
    std::size_t index_count;
    std::size_t vertex_offset;
};

struct material
{
    render_pipeline* pipeline;
    rhi_pipeline_parameter* parameter;
};

class mesh
{
public:
    mesh();

    void set_index_buffer(rhi_resource* index_buffer);
    void set_vertex_buffer(const std::string& name, rhi_resource* vertex_buffer);
    void remove_vertex_buffer(const std::string& name);

    std::size_t add_submesh(const submesh& submesh);
    void set_submesh(std::size_t submesh_index, const submesh& submesh);

    void set_material(std::size_t submesh_index, const material& material);

    node_parameter* get_node_parameter() const noexcept { return m_node_parameter.get(); }

    template <typename Functor>
    void each_submesh(Functor functor)
    {
        for (std::size_t i = 0; i < m_submeshes.size(); ++i)
        {
            auto& [material, vertex_buffers] = m_materials[i];
            submesh& submesh = m_submeshes[i];

            if (material.pipeline == nullptr)
                continue;

            functor(submesh, material, vertex_buffers, m_index_buffer);
        }
    }

private:
    std::unordered_map<std::string, rhi_resource*> m_vertex_buffers;
    rhi_resource* m_index_buffer;

    std::vector<submesh> m_submeshes;
    std::vector<std::pair<material, std::vector<rhi_resource*>>> m_materials;

    std::unique_ptr<node_parameter> m_node_parameter;
};
} // namespace violet