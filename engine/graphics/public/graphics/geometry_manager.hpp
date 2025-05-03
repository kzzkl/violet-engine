#pragma once

#include "common/allocator.hpp"
#include "graphics/geometry.hpp"
#include "graphics/resources/persistent_buffer.hpp"
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

    void update(gpu_buffer_uploader* uploader);

    void set_buffer(
        render_id geometry_id,
        geometry_buffer_type type,
        const void* data,
        std::size_t size);
    void set_shared_buffer(
        render_id dst_geometry_id,
        render_id src_geometry_id,
        geometry_buffer_type type);

    void mark_dirty(render_id geometry_id);

    std::uint32_t get_buffer_address(render_id geometry_id, geometry_buffer_type type);
    std::array<std::uint32_t, GEOMETRY_BUFFER_COUNT> get_buffer_addresses(render_id geometry_id);

    std::size_t get_buffer_size(render_id geometry_id, geometry_buffer_type type);

    raw_buffer* get_vertex_buffer()
    {
        return m_vertex_buffer.get();
    }

    raw_buffer* get_index_buffer()
    {
        return m_index_buffer.get();
    }

private:
    struct geometry_buffer
    {
        buffer_allocation allocation;

        std::size_t offset;
        std::size_t size;
    };

    struct geometry_info
    {
        geometry* geometry;
        std::array<geometry_buffer, GEOMETRY_BUFFER_COUNT> buffers;
    };

    std::vector<geometry_info> m_geometries;
    index_allocator<render_id> m_allocator;

    std::vector<render_id> m_dirty_geometries;

    std::unique_ptr<persistent_buffer> m_vertex_buffer;
    std::unique_ptr<persistent_buffer> m_index_buffer;

    std::mutex m_mutex;
};
} // namespace violet