#include "graphics/material_manager.hpp"

namespace violet
{
material_manager::material_manager(std::size_t material_buffer_size)
    : m_material_buffer_allocator(material_buffer_size)
{
    auto& device = render_device::instance();

    m_material_buffer = device.create_buffer({
        .data = nullptr,
        .size = material_buffer_size,
        .flags = RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST,
    });
}

render_id material_manager::register_material(material* material, std::uint32_t& constant_address)
{
    std::lock_guard lock(m_mutex);

    render_id material_id = m_material_allocator.allocate();
    if (m_materials.size() <= material_id)
    {
        m_materials.resize(material_id + 1);
    }

    m_materials[material_id] = {
        .material = material,
    };

    if (material->get_constant_size() != 0)
    {
        buffer_allocation allocation =
            m_material_buffer_allocator.allocate(material->get_constant_size());

        m_materials[material_id].constant_allocation = allocation;
        constant_address = allocation.offset;
    }

    return material_id;
}

void material_manager::unregister_material(render_id material_id)
{
    std::lock_guard lock(m_mutex);

    if (m_materials[material_id].material->get_constant_size() != 0)
    {
        m_material_buffer_allocator.free(m_materials[material_id].constant_allocation);
    }

    m_materials[material_id] = {};
    m_material_allocator.free(material_id);
}

bool material_manager::update()
{
    if (m_dirty_materials.empty())
    {
        return false;
    }

    for (render_id material_id : m_dirty_materials)
    {
        auto& material_info = m_materials[material_id];
        if (material_info.material != nullptr)
        {
            m_gpu_buffer_uploader.upload(
                m_material_buffer.get(),
                material_info.material->get_constant_data(),
                material_info.material->get_constant_size(),
                material_info.material->get_constant_address());

            material_info.material->clear_dirty();
        }
    }

    m_dirty_materials.clear();

    return true;
}

void material_manager::record(rhi_command* command)
{
    rhi_buffer_barrier barrier = {
        .buffer = m_material_buffer.get(),
        .src_access = RHI_ACCESS_SHADER_READ,
        .dst_access = RHI_ACCESS_TRANSFER_WRITE,
        .offset = 0,
        .size = m_material_buffer->get_buffer_size(),
    };

    command->set_pipeline_barrier(
        RHI_PIPELINE_STAGE_COMPUTE | RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_FRAGMENT,
        RHI_PIPELINE_STAGE_TRANSFER,
        &barrier,
        1,
        nullptr,
        0);

    m_gpu_buffer_uploader.record(command);

    barrier.src_access = RHI_ACCESS_TRANSFER_WRITE;
    barrier.dst_access = RHI_ACCESS_SHADER_READ;

    command->set_pipeline_barrier(
        RHI_PIPELINE_STAGE_TRANSFER,
        RHI_PIPELINE_STAGE_COMPUTE | RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_FRAGMENT,
        &barrier,
        1,
        nullptr,
        0);
}

void material_manager::mark_dirty(material* material)
{
    std::lock_guard lock(m_mutex);
    m_dirty_materials.push_back(material->get_id());
}
} // namespace violet