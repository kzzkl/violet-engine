#pragma once

#include "graphics/geometry.hpp"
#include "graphics/material/material.hpp"
#include "graphics/node_parameter.hpp"
#include "graphics/render_graph/render_pipeline.hpp"
#include <memory>
#include <vector>

namespace violet
{
struct submesh
{
    std::size_t index_begin;
    std::size_t index_end;
    std::size_t vertex_offset;

    std::size_t material_index;
};

class mesh
{
public:
    mesh();

    void set_geometry(geometry* geometry);

    void set_submesh(std::size_t index, const submesh& submesh);
    void set_submesh_count(std::size_t count);

    void set_material(std::size_t index, material* material);
    material* get_material(std::size_t index);

    void set_material_count(std::size_t count);
    std::size_t get_material_count() const noexcept;

    node_parameter* get_node_parameter() const noexcept { return m_node_parameter.get(); }

    using call = void (*)(
        const submesh& submesh,
        rhi_render_pipeline* pipeline,
        rhi_pipeline_parameter* material_parameter,
        const std::vector<rhi_resource*>& vertex_attributes,
        std::size_t vertex_attribute_hash);

    template <typename Functor>
    void each_render_unit(Functor&& functor) const
    {
        for (const submesh& submesh : m_submeshes)
        {
            const auto& material_wrapper = m_materials[submesh.material_index];
            const auto& pipelines = material_wrapper.material->get_pipelines();

            for (std::size_t i = 0; i < pipelines.size(); ++i)
            {
                functor(
                    submesh,
                    pipelines[i].first->get_interface(),
                    pipelines[i].second->get_interface(),
                    material_wrapper.vertex_attributes[i],
                    material_wrapper.vertex_attribute_hash[i]);
            }
        }
    }

private:
    struct material_wrapper
    {
        material* material;
        std::vector<std::vector<rhi_resource*>> vertex_attributes;
        std::vector<std::size_t> vertex_attribute_hash;
    };

    geometry* m_geometry;

    std::vector<submesh> m_submeshes;
    std::vector<material_wrapper> m_materials;

    std::unique_ptr<node_parameter> m_node_parameter;
};
} // namespace violet