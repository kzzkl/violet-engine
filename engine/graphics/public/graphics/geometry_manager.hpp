#pragma once

#include "common/allocator.hpp"
#include "gpu_array.hpp"
#include "graphics/geometry.hpp"
#include "graphics/resources/persistent_buffer.hpp"
#include "math/box.hpp"
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

        return static_cast<std::uint32_t>(buffer.offset + buffer.stride * offset);
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

        box3f bounding_box;
        sphere3f bounding_sphere;
        bool bounds_dirty;
    };

    sphere3f get_bounding_sphere(render_id submesh_id);

    std::vector<geometry_info> m_geometries;
    index_allocator<render_id> m_allocator;

    std::vector<render_id> m_dirty_geometries;

    std::unique_ptr<persistent_buffer> m_vertex_buffer;
    std::unique_ptr<persistent_buffer> m_index_buffer;

    gpu_sparse_array<gpu_geometry> m_submeshes;

    std::mutex m_mutex;
};
} // namespace violet