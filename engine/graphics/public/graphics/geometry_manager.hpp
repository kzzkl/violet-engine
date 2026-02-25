#pragma once

#include "common/allocator.hpp"
#include "gpu_array.hpp"
#include "graphics/geometry.hpp"
#include "graphics/resources/persistent_buffer.hpp"
#include "math/sphere.hpp"
#include <mutex>

namespace violet
{
class gpu_buffer_uploader;
class gpu_geometry_buffer;

class geometry_manager
{
public:
    geometry_manager();
    ~geometry_manager();

    render_id add_geometry(geometry* geometry);
    void remove_geometry(render_id geometry_id);

    render_id add_submesh(render_id geometry_id);
    void remove_submesh(render_id submesh_id);
    void set_submesh(
        render_id submesh_id,
        std::uint32_t vertex_offset,
        std::uint32_t index_offset,
        std::uint32_t index_count);
    void set_submesh(
        render_id submesh_id,
        std::span<const cluster> clusters,
        std::span<const cluster_node> cluster_nodes);

    void update(gpu_buffer_uploader* uploader);

    void set_buffer(
        render_id geometry_id,
        geometry_buffer_type type,
        const void* data,
        std::size_t size,
        std::size_t stride);
    void set_shared_buffer(
        render_id dst_geometry_id,
        render_id src_geometry_id,
        geometry_buffer_type type);

    void mark_dirty(render_id geometry_id);

    raw_buffer* get_vertex_buffer() const noexcept
    {
        return m_vertex_buffer.get();
    }

    raw_buffer* get_index_buffer() const noexcept
    {
        return m_index_buffer.get();
    }

    raw_buffer* get_geometry_buffer() const noexcept
    {
        return m_submeshes.get_buffer();
    }

    raw_buffer* get_cluster_buffer() const noexcept
    {
        return m_clusters.get_buffer();
    }

    raw_buffer* get_cluster_node_buffer() const noexcept
    {
        return m_cluster_nodes.get_buffer();
    }

    std::uint32_t get_cluster_count() const noexcept
    {
        return m_clusters.get_size();
    }

    std::uint32_t get_cluster_capacity() const noexcept
    {
        return m_clusters.get_capacity();
    }

    std::uint32_t get_cluster_node_depth() const noexcept
    {
        return m_cluster_node_depth;
    }

    std::uint32_t get_buffer_address(
        render_id geometry_id,
        geometry_buffer_type type,
        std::uint32_t offset = 0) const
    {
        const auto& buffer = m_geometries[geometry_id].buffers[type];

        if (buffer.src_geometry_id != INVALID_RENDER_ID)
        {
            return geometry_manager::get_buffer_address(buffer.src_geometry_id, type, offset);
        }

        return static_cast<std::uint32_t>(buffer.offset + (buffer.stride * offset));
    }

    std::uint32_t get_buffer_size(render_id geometry_id, geometry_buffer_type type) const
    {
        return static_cast<std::uint32_t>(m_geometries[geometry_id].buffers[type].size);
    }

private:
    struct geometry_buffer
    {
        buffer_allocation allocation;

        std::uint32_t offset;
        std::size_t size;
        std::size_t stride;

        render_id src_geometry_id{INVALID_RENDER_ID};
    };

    struct geometry_info
    {
        geometry* geometry;
        std::array<geometry_buffer, GEOMETRY_BUFFER_COUNT> buffers;
    };

    struct gpu_geometry
    {
        using gpu_type = shader::geometry_data;

        render_id submesh_id;
        render_id geometry_id;
        std::uint32_t vertex_offset;
        std::uint32_t index_offset;
        std::uint32_t index_count;

        sphere3f bounding_sphere;

        render_id cluster_root_id{INVALID_RENDER_ID};
    };

    struct gpu_cluster
    {
        struct gpu_type
        {
            vec4f bounding_sphere;
            vec4f lod_bounds;
            float lod_error;
            std::uint32_t index_offset;
            std::uint32_t index_count;
            std::uint32_t cluster_node;
        };

        std::uint32_t index_offset;
        std::uint32_t index_count;

        sphere3f bounding_sphere;

        sphere3f lod_bounds;
        float lod_error;

        std::uint32_t cluster_node;
    };

    struct gpu_cluster_node
    {
        struct gpu_type
        {
            vec4f bounding_sphere;
            vec4f lod_bounds;

            float min_lod_error;
            float max_parent_lod_error;

            std::uint32_t is_leaf;
            std::uint32_t child_offset;
            std::uint32_t child_count;

            std::uint32_t padding0;
            std::uint32_t padding1;
            std::uint32_t padding2;
        };

        sphere3f bounding_sphere;

        sphere3f lod_bounds;
        float min_lod_error;
        float max_parent_lod_error;

        bool is_leaf;
        std::uint32_t depth;
        std::uint32_t child_offset;
        std::uint32_t child_count;
    };

    std::vector<geometry_info> m_geometries;
    index_allocator m_allocator;

    std::vector<render_id> m_dirty_geometries;

    std::unique_ptr<persistent_buffer> m_vertex_buffer;
    std::unique_ptr<persistent_buffer> m_index_buffer;

    gpu_sparse_array<gpu_geometry> m_submeshes;
    gpu_block_sparse_array<gpu_cluster> m_clusters;
    gpu_block_sparse_array<gpu_cluster_node> m_cluster_nodes;
    std::uint32_t m_cluster_node_depth{0};

    std::mutex m_mutex;
};
} // namespace violet