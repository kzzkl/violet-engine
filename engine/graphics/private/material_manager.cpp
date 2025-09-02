#include "graphics/material_manager.hpp"
#include "gpu_buffer_uploader.hpp"

namespace violet
{
material_manager::material_manager(std::size_t material_buffer_size)
    : m_material_resolve_pipeline_id_allocator(1, 0xFFFFFF)
{
    m_material_buffer = std::make_unique<persistent_buffer>(
        material_buffer_size,
        RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);
}

material_manager::~material_manager() {}

render_id material_manager::add_material(material* material)
{
    std::scoped_lock lock(m_mutex);

    render_id material_id = m_material_allocator.allocate();
    if (m_materials.size() <= material_id)
    {
        m_materials.resize(material_id + 1);
    }

    m_materials[material_id] = {
        .material = material,
    };

    return material_id;
}

void material_manager::remove_material(render_id material_id)
{
    std::scoped_lock lock(m_mutex);

    if (m_materials[material_id].constant_allocation.offset != buffer_allocator::no_space)
    {
        m_material_buffer->free(m_materials[material_id].constant_allocation);
    }

    m_materials[material_id] = {};
    m_material_allocator.free(material_id);
}

render_id material_manager::add_material_resolve_pipeline(const rdg_compute_pipeline& pipeline)
{
    std::scoped_lock lock(m_mutex);

    auto iter = m_material_resolve_pipeline_ids.find(pipeline.compute_shader);
    if (iter != m_material_resolve_pipeline_ids.end())
    {
        ++m_material_resolve_pipelines[iter->second].reference_count;
        return iter->second;
    }

    render_id pipeline_id = m_material_resolve_pipeline_id_allocator.allocate();
    if (pipeline_id == buffer_allocator::no_space)
    {
        assert(false);
        return 0;
    }

    if (m_material_resolve_pipelines.size() <= pipeline_id)
    {
        m_material_resolve_pipelines.resize(pipeline_id + 1);
    }

    m_material_resolve_pipelines[pipeline_id] = {
        .pipeline = pipeline,
        .reference_count = 1,
    };

    m_material_resolve_pipeline_ids[pipeline.compute_shader] = pipeline_id;

    m_max_material_resolve_pipeline_id = std::max(m_max_material_resolve_pipeline_id, pipeline_id);

    return pipeline_id;
}

void material_manager::remove_material_resolve_pipeline(render_id pipeline_id)
{
    std::scoped_lock lock(m_mutex);

    assert(
        m_material_resolve_pipelines.size() > pipeline_id &&
        m_material_resolve_pipelines[pipeline_id].reference_count > 0);

    auto& wrapper = m_material_resolve_pipelines.at(pipeline_id);

    --wrapper.reference_count;
    if (wrapper.reference_count == 0)
    {
        m_material_resolve_pipeline_ids.erase(wrapper.pipeline.compute_shader);
        wrapper = {};
        m_material_resolve_pipeline_id_allocator.free(pipeline_id);
    }

    if (pipeline_id == m_max_material_resolve_pipeline_id)
    {
        while (m_max_material_resolve_pipeline_id > 0 &&
               m_material_resolve_pipelines[m_max_material_resolve_pipeline_id].reference_count ==
                   0)
        {
            --m_max_material_resolve_pipeline_id;
        }
    }
}

const rdg_compute_pipeline& material_manager::get_material_resolve_pipeline(
    render_id pipeline_id) const
{
    return m_material_resolve_pipelines.at(pipeline_id).pipeline;
}

void material_manager::set_shading_model(
    render_id shading_model_id,
    std::unique_ptr<shading_model_base>&& shading_model)
{
    std::scoped_lock lock(m_mutex);
    if (m_shading_models.size() <= shading_model_id)
    {
        m_shading_models.resize(shading_model_id + 1);
    }

    m_shading_models[shading_model_id] = std::move(shading_model);
}

shading_model_base* material_manager::get_shading_model(render_id shading_model_id) const noexcept
{
    std::scoped_lock lock(m_mutex);

    if (m_shading_models.size() <= shading_model_id ||
        m_shading_models[shading_model_id] == nullptr)
    {
        return nullptr;
    }

    return m_shading_models[shading_model_id].get();
}

void material_manager::update(gpu_buffer_uploader* uploader)
{
    if (m_dirty_materials.empty())
    {
        return;
    }

    for (render_id material_id : m_dirty_materials)
    {
        auto& material_info = m_materials[material_id];
        if (material_info.material != nullptr)
        {
            material_info.material->update();
        }
    }

    m_dirty_materials.clear();

    m_material_buffer->upload(
        uploader,
        RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_FRAGMENT | RHI_PIPELINE_STAGE_COMPUTE,
        RHI_ACCESS_SHADER_READ);
}

void material_manager::update_constant(render_id material_id, const void* data, std::size_t size)
{
    auto& material_info = m_materials[material_id];

    if (material_info.constant_allocation.offset == buffer_allocator::no_space)
    {
        material_info.constant_allocation = m_material_buffer->allocate(size);
        material_info.constant_size = size;
    }
    else if (material_info.constant_size < size)
    {
        m_material_buffer->free(material_info.constant_allocation);
        material_info.constant_allocation = m_material_buffer->allocate(size);
        material_info.constant_size = size;
    }

    m_material_buffer->copy(data, size, material_info.constant_allocation.offset);
}

void material_manager::mark_dirty(render_id material_id)
{
    std::scoped_lock lock(m_mutex);
    m_dirty_materials.push_back(material_id);
}
} // namespace violet