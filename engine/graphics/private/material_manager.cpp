#include "graphics/material_manager.hpp"
#include "gpu_buffer_uploader.hpp"

namespace violet
{
material_manager::material_manager(std::size_t material_buffer_size)
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