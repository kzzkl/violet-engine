#include "graphics/geometry_manager.hpp"
#include "gpu_buffer_uploader.hpp"
#include "graphics/extensions/ray_tracing_extension.hpp"
#include <queue>

namespace violet
{
geometry_manager::geometry_manager()
    : m_clusters(16 * 1024 * 1024),
      m_cluster_nodes(2 * 1024 * 1024)
{
    auto& device = render_device::instance();

    rhi_buffer_flags flags = 0;
    flags |= device.has_extension<ray_tracing_extension>() ?
                 RHI_BUFFER_DEVICE_ADDRESS | RHI_BUFFER_ACCELERATION_STRUCTURE_BUILD_INPUT :
                 0;

    m_vertex_buffer = std::make_unique<persistent_buffer>(
        100 * 1024 * 1024,
        flags | RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);

    m_index_buffer = std::make_unique<persistent_buffer>(
        100 * 1024 * 1024,
        flags | RHI_BUFFER_INDEX | RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);
}

geometry_manager::~geometry_manager() {}

render_id geometry_manager::add_geometry(geometry* geometry)
{
    std::scoped_lock lock(m_mutex);

    render_id geometry_id = m_allocator.allocate();

    if (geometry_id >= m_geometries.size())
    {
        m_geometries.resize(geometry_id + 1);
    }

    m_geometries[geometry_id] = {};
    m_geometries[geometry_id].geometry = geometry;

    return geometry_id;
}

void geometry_manager::remove_geometry(render_id geometry_id)
{
    std::scoped_lock lock(m_mutex);

    for (std::uint32_t type = 0; type < GEOMETRY_BUFFER_COUNT; ++type)
    {
        auto& buffer = m_geometries[geometry_id].buffers[type];

        if (buffer.allocation.offset == buffer_allocator::no_space)
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

render_id geometry_manager::add_submesh(
    render_id geometry_id,
    std::uint32_t vertex_offset,
    std::uint32_t index_offset,
    std::uint32_t index_count)
{
    std::scoped_lock lock(m_mutex);

    render_id submesh_id = m_submeshes.add();

    auto& submesh = m_submeshes[submesh_id];
    submesh.submesh_id = submesh_id;
    submesh.geometry_id = geometry_id;
    submesh.vertex_offset = vertex_offset;
    submesh.index_offset = index_offset;
    submesh.index_count = index_count;

    geometry* geometry = m_geometries[submesh.geometry_id].geometry;

    auto positions = geometry->get_positions();
    auto indexes = geometry->get_indexes();

    std::vector<vec3f> points;
    points.reserve(submesh.index_count);
    for (std::size_t i = 0; i < submesh.index_count; ++i)
    {
        points.push_back(positions[indexes[submesh.index_offset + i] + submesh.vertex_offset]);
    }
    submesh.bounding_box = box::create(points);
    submesh.bounding_sphere = sphere::create(points);

    m_submeshes.mark_dirty(submesh_id);

    return submesh_id;
}

render_id geometry_manager::add_submesh(
    render_id geometry_id,
    std::span<const cluster> clusters,
    std::span<const cluster_node> cluster_nodes)
{
    assert(!clusters.empty() && !cluster_nodes.empty());

    std::uint32_t cluster_count = 0;
    std::uint32_t cluster_node_count = 1; // Root

    for (const auto& cluster_node : cluster_nodes)
    {
        if (cluster_node.is_leaf)
        {
            cluster_count += cluster_node.child_count;
        }
        else
        {
            cluster_node_count += cluster_node.child_count;
        }
    }

    std::scoped_lock lock(m_mutex);

    std::uint32_t cluster_offset = m_clusters.add(cluster_count);
    std::uint32_t cluster_node_offset = m_cluster_nodes.add(cluster_node_count);

    render_id submesh_id = m_submeshes.add();

    auto& submesh = m_submeshes[submesh_id];
    submesh.submesh_id = submesh_id;
    submesh.geometry_id = geometry_id;
    submesh.bounding_box = cluster_nodes[0].bounding_box;
    submesh.bounding_sphere = cluster_nodes[0].bounding_sphere;
    submesh.cluster_id = cluster_offset;
    submesh.cluster_node_id = cluster_node_offset;

    std::queue<std::pair<std::uint32_t, render_id>> queue;
    queue.emplace(0, cluster_node_offset);
    ++cluster_node_offset;

    while (!queue.empty())
    {
        auto [index, id] = queue.front();
        queue.pop();

        const auto& cluster_node = cluster_nodes[index];
        assert(cluster_node.child_count != 0);

        m_cluster_nodes[id] = {
            .bounding_sphere = cluster_node.bounding_sphere,
            .lod_bounds = cluster_node.lod_bounds,
            .min_lod_error = cluster_node.min_lod_error,
            .max_parent_lod_error = cluster_node.max_parent_lod_error,
            .is_leaf = cluster_node.is_leaf,
            .depth = cluster_node.depth,
            .child_count = cluster_node.child_count,
        };

        if (cluster_node.is_leaf)
        {
            for (std::uint32_t i = 0; i < cluster_node.child_count; ++i)
            {
                const auto& cluster = clusters[cluster_node.child_offset + i];
                m_clusters[cluster_offset + i] = {
                    .index_offset = cluster.index_offset,
                    .index_count = cluster.index_count,
                    .bounding_sphere = cluster.bounding_sphere,
                    .lod_bounds = cluster.lod_bounds,
                    .lod_error = cluster.lod == 0 ? -1.0f : cluster.lod_error,
                    .cluster_node = id,
                };
            }

            m_cluster_nodes[id].child_offset = cluster_offset;
            cluster_offset += cluster_node.child_count;
        }
        else if (cluster_node.child_count != 0)
        {
            for (std::uint32_t i = 0; i < cluster_node.child_count; ++i)
            {
                queue.emplace(cluster_node.child_offset + i, cluster_node_offset + i);
            }

            m_cluster_nodes[id].child_offset = cluster_node_offset;
            cluster_node_offset += cluster_node.child_count;
        }
    }

    m_submeshes.mark_dirty(submesh_id);

    return submesh_id;
}

void geometry_manager::remove_submesh(render_id submesh_id)
{
    std::scoped_lock lock(m_mutex);

    auto& submesh = m_submeshes[submesh_id];

    if (submesh.cluster_id != INVALID_RENDER_ID)
    {
        m_clusters.remove(submesh.cluster_id);
        submesh.cluster_id = INVALID_RENDER_ID;
    }

    if (submesh.cluster_node_id != INVALID_RENDER_ID)
    {
        m_cluster_nodes.remove(submesh.cluster_node_id);
        submesh.cluster_node_id = INVALID_RENDER_ID;
    }

    m_submeshes.remove(submesh_id);
}

box3f geometry_manager::get_bounding_box(render_id submesh_id) const
{
    return m_submeshes[submesh_id].bounding_box;
}

sphere3f geometry_manager::get_bounding_sphere(render_id submesh_id) const
{
    return m_submeshes[submesh_id].bounding_sphere;
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
        if (geometry_buffer.allocation.offset == buffer_allocator::no_space)
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
        if (geometry_buffer.allocation.offset == buffer_allocator::no_space)
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

std::size_t geometry_manager::get_buffer_offset(render_id geometry_id, geometry_buffer_type type)
    const
{
    auto& geometry_buffer = m_geometries[geometry_id].buffers[type];
    return geometry_buffer.offset;
}

void geometry_manager::set_shared_buffer(
    render_id dst_geometry_id,
    render_id src_geometry_id,
    geometry_buffer_type type)
{
    auto& dst_buffer = m_geometries[dst_geometry_id].buffers[type];

    if (dst_buffer.allocation.offset != buffer_allocator::no_space)
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

void geometry_manager::upload(gpu_buffer_uploader* uploader)
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
                .uv_address = get_buffer_address(
                    geometry.geometry_id,
                    GEOMETRY_BUFFER_UV,
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
                    (get_buffer_address(geometry.geometry_id, GEOMETRY_BUFFER_INDEX) / 4) +
                    geometry.index_offset,
                .index_count = geometry.index_count,
                .cluster_root = static_cast<std::uint32_t>(geometry.cluster_node_id),
                .bounding_box_min = geometry.bounding_box.min,
                .bounding_box_max = geometry.bounding_box.max,
                .bounding_sphere = m_submeshes[geometry.submesh_id].bounding_sphere,
            };
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            rhi_buffer_region region = {
                .offset = offset,
                .size = size,
            };

            uploader->upload(
                buffer,
                data,
                size,
                region,
                RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });

    m_clusters.update(
        [&](const gpu_cluster& cluster) -> gpu_cluster::gpu_type
        {
            return {
                .bounding_sphere = cluster.bounding_sphere,
                .lod_bounds = cluster.lod_bounds,
                .lod_error = cluster.lod_error,
                .index_offset = cluster.index_offset,
                .index_count = cluster.index_count,
                .cluster_node = cluster.cluster_node,
            };
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            rhi_buffer_region region = {
                .offset = offset,
                .size = size,
            };

            uploader->upload(
                buffer,
                data,
                size,
                region,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });

    m_cluster_nodes.update(
        [&](const gpu_cluster_node& cluster_node) -> gpu_cluster_node::gpu_type
        {
            return {
                .bounding_sphere = cluster_node.bounding_sphere,
                .lod_bounds = cluster_node.lod_bounds,
                .min_lod_error = cluster_node.min_lod_error,
                .max_parent_lod_error = cluster_node.max_parent_lod_error,
                .is_leaf = cluster_node.is_leaf,
                .child_offset = cluster_node.child_offset,
                .child_count = cluster_node.child_count,
            };
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            rhi_buffer_region region = {
                .offset = offset,
                .size = size,
            };

            uploader->upload(
                buffer,
                data,
                size,
                region,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });

    m_cluster_node_depth = 0;
    m_cluster_nodes.each(
        [&](render_id id, const gpu_cluster_node& cluster_node)
        {
            m_cluster_node_depth = std::max(m_cluster_node_depth, cluster_node.depth);
        });
    ++m_cluster_node_depth;
}

void geometry_manager::mark_dirty(render_id geometry_id)
{
    std::scoped_lock lock(m_mutex);

    m_dirty_geometries.push_back(geometry_id);
}
} // namespace violet