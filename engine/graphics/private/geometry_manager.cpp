#include "graphics/geometry_manager.hpp"
#include "gpu_buffer_uploader.hpp"

namespace violet
{
geometry_manager::geometry_manager()
{
    m_vertex_buffer = std::make_unique<persistent_buffer>(
        1024 * 1024,
        RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);

    m_index_buffer = std::make_unique<persistent_buffer>(
        1024 * 1024,
        RHI_BUFFER_INDEX | RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);
}

geometry_manager::~geometry_manager() {}

render_id geometry_manager::add_geometry(geometry* geometry)
{
    std::lock_guard lock(m_mutex);

    render_id geometry_id = m_allocator.allocate();

    if (geometry_id >= m_geometries.size())
    {
        m_geometries.resize(geometry_id + 1);
    }

    m_geometries[geometry_id].geometry = geometry;

    return geometry_id;
}

void geometry_manager::remove_geometry(render_id geometry_id)
{
    std::lock_guard lock(m_mutex);
    m_allocator.free(geometry_id);
}

void geometry_manager::update(gpu_buffer_uploader* uploader)
{
    if (m_dirty_geometries.empty())
    {
        return;
    }

    for (render_id geometry_id : m_dirty_geometries)
    {
        geometry_info& geometry_info = m_geometries[geometry_id];
        geometry_info.geometry->update();
    }

    m_dirty_geometries.clear();

    m_vertex_buffer->upload(uploader, RHI_PIPELINE_STAGE_VERTEX, RHI_ACCESS_SHADER_READ);
    m_index_buffer->upload(uploader, RHI_PIPELINE_STAGE_VERTEX_INPUT, RHI_ACCESS_INDEX_READ);
}

void geometry_manager::set_buffer(
    render_id geometry_id,
    geometry_buffer_type type,
    const void* data,
    std::size_t size)
{
    auto& geometry_buffer = m_geometries[geometry_id].buffers[type];

    if (type != GEOMETRY_BUFFER_INDEX)
    {
        if (geometry_buffer.allocation.offset == buffer_allocation::NO_SPACE)
        {
            geometry_buffer.allocation = m_vertex_buffer->allocate(size);
            geometry_buffer.offset = geometry_buffer.allocation.offset;
            geometry_buffer.size = size;
        }
        else if (geometry_buffer.size < size)
        {
            m_vertex_buffer->free(geometry_buffer.allocation);
            geometry_buffer.allocation = m_vertex_buffer->allocate(size);
            geometry_buffer.offset = geometry_buffer.allocation.offset;
            geometry_buffer.size = size;
        }

        m_vertex_buffer->copy(data, size, geometry_buffer.offset);
    }
    else
    {
        if (geometry_buffer.allocation.offset == buffer_allocation::NO_SPACE)
        {
            geometry_buffer.allocation = m_index_buffer->allocate(size);
            geometry_buffer.offset = geometry_buffer.allocation.offset;
            geometry_buffer.size = size;
        }
        else if (geometry_buffer.size < size)
        {
            m_index_buffer->free(geometry_buffer.allocation);
            geometry_buffer.allocation = m_index_buffer->allocate(size);
            geometry_buffer.offset = geometry_buffer.allocation.offset;
            geometry_buffer.size = size;
        }

        m_index_buffer->copy(data, size, geometry_buffer.offset);
    }
}

void geometry_manager::set_shared_buffer(
    render_id dst_geometry_id,
    render_id src_geometry_id,
    geometry_buffer_type type)
{
    auto& src_buffer = m_geometries[src_geometry_id].buffers[type];
    auto& dst_buffer = m_geometries[dst_geometry_id].buffers[type];

    if (dst_buffer.allocation.offset != buffer_allocation::NO_SPACE)
    {
        if (type == GEOMETRY_BUFFER_INDEX)
        {
            m_index_buffer->free(dst_buffer.allocation);
        }
        else
        {
            m_vertex_buffer->free(dst_buffer.allocation);
        }

        dst_buffer.allocation = {};
    }

    dst_buffer.offset = src_buffer.offset;
    dst_buffer.size = src_buffer.size;
}

void geometry_manager::mark_dirty(render_id geometry_id)
{
    std::lock_guard lock(m_mutex);
    m_dirty_geometries.push_back(geometry_id);
}

std::uint32_t geometry_manager::get_buffer_address(render_id geometry_id, geometry_buffer_type type)
{
    auto& geometry_buffer = m_geometries[geometry_id].buffers[type];
    return static_cast<std::uint32_t>(geometry_buffer.offset);
}

std::array<std::uint32_t, GEOMETRY_BUFFER_COUNT> geometry_manager::get_buffer_addresses(
    render_id geometry_id)
{
    std::array<std::uint32_t, GEOMETRY_BUFFER_COUNT> result;

    auto& geometry_info = m_geometries[geometry_id];

    for (std::size_t i = 0; i < GEOMETRY_BUFFER_COUNT; ++i)
    {
        result[i] = static_cast<std::uint32_t>(geometry_info.buffers[i].offset);
    }

    return result;
}

std::size_t geometry_manager::get_buffer_size(render_id geometry_id, geometry_buffer_type type)
{
    auto& geometry_buffer = m_geometries[geometry_id].buffers[type];
    return geometry_buffer.size;
}
} // namespace violet