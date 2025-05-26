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

    for (std::uint32_t type = 0; type < GEOMETRY_BUFFER_COUNT; ++type)
    {
        auto& buffer = m_geometries[geometry_id].buffers[type];

        if (buffer.allocation.offset == buffer_allocation::NO_SPACE)
        {
            continue;
        }

        if (type == GEOMETRY_BUFFER_INDEX)
        {
            m_index_buffer->free(buffer.allocation);
        }
        else
        {
            m_vertex_buffer->free(buffer.allocation);
        }

        buffer = {};
    }

    m_geometries[geometry_id].geometry = nullptr;
    m_allocator.free(geometry_id);
}

render_id geometry_manager::add_submesh(render_id geometry_id)
{
    std::lock_guard lock(m_mutex);

    render_id submesh_id = m_submeshes.add();
    m_submeshes[submesh_id].submesh_id = submesh_id;
    m_submeshes[submesh_id].geometry_id = geometry_id;

    return submesh_id;
}

void geometry_manager::remove_submesh(render_id submesh_id)
{
    std::lock_guard lock(m_mutex);

    m_submeshes.remove(submesh_id);
}

void geometry_manager::set_submesh(
    render_id submesh_id,
    std::uint32_t vertex_offset,
    std::uint32_t index_offset,
    std::uint32_t index_count)
{
    auto& submesh = m_submeshes[submesh_id];

    if (submesh.vertex_offset == vertex_offset && submesh.index_offset == index_offset &&
        submesh.index_count == index_count)
    {
        return;
    }

    submesh.vertex_offset = vertex_offset;
    submesh.index_offset = index_offset;
    submesh.index_count = index_count;
    submesh.bounds_dirty = true;

    m_submeshes.mark_dirty(submesh_id);
}

void geometry_manager::update(gpu_buffer_uploader* uploader)
{
    if (m_dirty_geometries.empty())
    {
        return;
    }

    for (render_id geometry_id : m_dirty_geometries)
    {
        geometry* geometry = m_geometries[geometry_id].geometry;
        if (geometry != nullptr)
        {
            geometry->update();
        }
    }

    m_dirty_geometries.clear();

    m_vertex_buffer->upload(uploader, RHI_PIPELINE_STAGE_VERTEX, RHI_ACCESS_SHADER_READ);
    m_index_buffer->upload(uploader, RHI_PIPELINE_STAGE_VERTEX_INPUT, RHI_ACCESS_INDEX_READ);

    m_submeshes.update(
        [&](const gpu_geometry& geometry) -> shader::geometry_data
        {
            return {
                .bounding_sphere = get_bounding_sphere(geometry.submesh_id),
                .position_address = get_buffer_address(
                    geometry.geometry_id,
                    GEOMETRY_BUFFER_POSITION,
                    geometry.vertex_offset),
                .normal_address = get_buffer_address(
                    geometry.geometry_id,
                    GEOMETRY_BUFFER_NORMAL,
                    geometry.vertex_offset),
                .tangent_address = get_buffer_address(
                    geometry.geometry_id,
                    GEOMETRY_BUFFER_TANGENT,
                    geometry.vertex_offset),
                .texcoord_address = get_buffer_address(
                    geometry.geometry_id,
                    GEOMETRY_BUFFER_TEXCOORD,
                    geometry.vertex_offset),
                .custom0_address = get_buffer_address(
                    geometry.geometry_id,
                    GEOMETRY_BUFFER_CUSTOM_0,
                    geometry.vertex_offset),
                .custom1_address = get_buffer_address(
                    geometry.geometry_id,
                    GEOMETRY_BUFFER_CUSTOM_1,
                    geometry.vertex_offset),
                .custom2_address = get_buffer_address(
                    geometry.geometry_id,
                    GEOMETRY_BUFFER_CUSTOM_2,
                    geometry.vertex_offset),
                .custom3_address = get_buffer_address(
                    geometry.geometry_id,
                    GEOMETRY_BUFFER_CUSTOM_3,
                    geometry.vertex_offset),
                .index_offset =
                    get_buffer_address(geometry.geometry_id, GEOMETRY_BUFFER_INDEX) / 4 +
                    geometry.index_offset,
                .index_count = geometry.index_count,
            };
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            uploader->upload(
                buffer,
                data,
                size,
                offset,
                RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });
}

void geometry_manager::set_buffer(
    render_id geometry_id,
    geometry_buffer_type type,
    const void* data,
    std::size_t size,
    std::size_t stride)
{
    if (size == 0)
    {
        return;
    }

    assert(data != nullptr && stride != 0);

    auto& geometry_buffer = m_geometries[geometry_id].buffers[type];
    geometry_buffer.stride = stride;

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

    dst_buffer.src_geometry_id = src_geometry_id;
}

void geometry_manager::mark_dirty(render_id geometry_id)
{
    std::lock_guard lock(m_mutex);
    m_dirty_geometries.push_back(geometry_id);
}

sphere3f geometry_manager::get_bounding_sphere(render_id submesh_id)
{
    if (m_submeshes[submesh_id].bounds_dirty)
    {
        geometry* geometry = m_geometries[m_submeshes[submesh_id].geometry_id].geometry;

        auto positions = geometry->get_positions();
        auto indexes = geometry->get_indexes();

        const auto& submesh = m_submeshes[submesh_id];

        box3f bounding_box = {};
        for (std::size_t i = 0; i < submesh.index_count; ++i)
        {
            box::expand(
                bounding_box,
                positions[indexes[submesh.index_offset + i] + submesh.vertex_offset]);
        }

        sphere3f bounding_sphere = {};
        bounding_sphere.center = box::get_center(bounding_box);
        bounding_sphere.radius = 0.0f;
        for (std::size_t i = 0; i < submesh.index_count; ++i)
        {
            sphere::expand(
                bounding_sphere,
                positions[indexes[submesh.index_offset + i] + submesh.vertex_offset]);
        }

        m_submeshes[submesh_id].bounding_box = bounding_box;
        m_submeshes[submesh_id].bounding_sphere = bounding_sphere;
        m_submeshes[submesh_id].bounds_dirty = false;
    }

    return m_submeshes[submesh_id].bounding_sphere;
}
} // namespace violet