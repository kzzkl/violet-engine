#include "graphics/geometry_manager.hpp"
#include "gpu_buffer_uploader.hpp"
#include <queue>

namespace violet
{
geometry_manager::geometry_manager()
    : m_clusters(24),     // max cluster count is 16M(2^24)
      m_cluster_nodes(21) // max cluster node count is 2M(2^21)
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
    std::scoped_lock lock(m_mutex);

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

render_id geometry_manager::add_submesh(render_id geometry_id)
{
    std::scoped_lock lock(m_mutex);

    render_id submesh_id = m_submeshes.add();
    m_submeshes[submesh_id].submesh_id = submesh_id;
    m_submeshes[submesh_id].geometry_id = geometry_id;

    return submesh_id;
}

void geometry_manager::remove_submesh(render_id submesh_id)
{
    std::scoped_lock lock(m_mutex);

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

    geometry* geometry = m_geometries[submesh.geometry_id].geometry;

    auto positions = geometry->get_positions();
    auto indexes = geometry->get_indexes();

    std::vector<vec3f> points;
    points.reserve(submesh.index_count);
    for (std::size_t i = 0; i < submesh.index_count; ++i)
    {
        points.push_back(positions[indexes[submesh.index_offset + i] + submesh.vertex_offset]);
    }
    submesh.bounding_sphere = sphere::create(points);

    m_submeshes.mark_dirty(submesh_id);
}

void geometry_manager::set_submesh(
    render_id submesh_id,
    std::span<const cluster> clusters,
    std::span<const cluster_node> cluster_nodes)
{
    render_id root_id = m_cluster_nodes.add();

    m_submeshes[submesh_id].cluster_root_id = root_id;
    m_submeshes[submesh_id].bounding_sphere = cluster_nodes[0].bounding_sphere;
    m_submeshes.mark_dirty(submesh_id);

    std::queue<std::pair<std::uint32_t, render_id>> queue;
    queue.emplace(0, root_id);

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
            render_id first_cluster_id = m_clusters.add(cluster_node.child_count);
            for (std::uint32_t i = 0; i < cluster_node.child_count; ++i)
            {
                const auto& cluster = clusters[cluster_node.child_offset + i];
                m_clusters[first_cluster_id + i] = {
                    .index_offset = cluster.index_offset,
                    .index_count = cluster.index_count,
                    .bounding_sphere = cluster.bounding_sphere,
                    .lod_bounds = cluster.lod_bounds,
                    .lod_error = cluster.lod == 0 ? -1.0f : cluster.lod_error,
                    .cluster_node = id,
                };
            }

            m_cluster_nodes[id].child_offset = first_cluster_id;
        }
        else if (cluster_node.child_count != 0)
        {
            render_id first_child_id = m_cluster_nodes.add(cluster_node.child_count);
            for (std::uint32_t i = 0; i < cluster_node.child_count; ++i)
            {
                queue.emplace(cluster_node.child_offset + i, first_child_id + i);
            }

            m_cluster_nodes[id].child_offset = first_child_id;
        }
    }
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
                .bounding_sphere = m_submeshes[geometry.submesh_id].bounding_sphere,
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
                    (get_buffer_address(geometry.geometry_id, GEOMETRY_BUFFER_INDEX) / 4) +
                    geometry.index_offset,
                .index_count = geometry.index_count,
                .cluster_root = static_cast<std::uint32_t>(geometry.cluster_root_id),
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
            uploader->upload(
                buffer,
                data,
                size,
                offset,
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
            uploader->upload(
                buffer,
                data,
                size,
                offset,
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

void geometry_manager::mark_dirty(render_id geometry_id)
{
    std::scoped_lock lock(m_mutex);

    m_dirty_geometries.push_back(geometry_id);
}
} // namespace violet