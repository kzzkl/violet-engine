#pragma once

#include "common/allocator.hpp"
#include "graphics/material.hpp"
#include "graphics/resources/persistent_buffer.hpp"
#include "graphics/shading_model.hpp"
#include <map>
#include <mutex>

namespace violet
{
class gpu_buffer_uploader;
class material_manager
{
public:
    material_manager(std::size_t material_buffer_size = 64ull * 1024);
    ~material_manager();

    render_id add_material(material* material);
    void remove_material(render_id material_id);

    render_id add_material_resolve_pipeline(const rdg_compute_pipeline& pipeline);
    void remove_material_resolve_pipeline(render_id pipeline_id);
    const rdg_compute_pipeline& get_material_resolve_pipeline(render_id pipeline_id) const;

    render_id get_max_material_resolve_pipeline_id() const noexcept
    {
        return m_max_material_resolve_pipeline_id;
    }

    void set_shading_model(
        render_id shading_model_id,
        std::unique_ptr<shading_model_base>&& shading_model);
    shading_model_base* get_shading_model(render_id shading_model_id) const noexcept;

    render_id get_max_shading_model_id() const noexcept
    {
        return m_shading_models.empty() ? 0 : static_cast<render_id>(m_shading_models.size() - 1);
    }

    void update(gpu_buffer_uploader* uploader);
    void update_constant(render_id material_id, const void* data, std::size_t size);

    void mark_dirty(render_id material_id);

    std::uint32_t get_material_constant_address(render_id material_id) const
    {
        return m_materials[material_id].constant_allocation.offset;
    }

    raw_buffer* get_material_buffer() const noexcept
    {
        return m_material_buffer.get();
    }

private:
    struct material_info
    {
        material* material;

        std::size_t constant_size;
        buffer_allocation constant_allocation;
    };

    struct pipeline_wrapper
    {
        rdg_compute_pipeline pipeline;
        std::uint32_t reference_count;
    };

    std::vector<material_info> m_materials;
    index_allocator m_material_allocator;
    std::vector<render_id> m_dirty_materials;

    std::vector<pipeline_wrapper> m_material_resolve_pipelines;
    std::map<rhi_shader*, render_id> m_material_resolve_pipeline_ids;
    index_allocator m_material_resolve_pipeline_id_allocator;
    render_id m_max_material_resolve_pipeline_id{0};

    std::vector<std::unique_ptr<shading_model_base>> m_shading_models;

    std::unique_ptr<persistent_buffer> m_material_buffer;

    mutable std::mutex m_mutex;
};
} // namespace violet