#pragma once

#include "common/allocator.hpp"
#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include "graphics/render_device.hpp"
#include <vector>

namespace violet
{
struct render_camera
{
    rhi_parameter* parameter;
    std::vector<rhi_texture*> render_targets;

    rhi_viewport viewport;
};

struct render_list
{
    struct mesh
    {
        rhi_parameter* transform;
        geometry* geometry;
    };

    struct draw_item
    {
        std::size_t mesh_index;
        std::size_t parameter_index;

        std::uint32_t vertex_start;
        std::uint32_t index_start;
        std::uint32_t index_count;
    };

    struct batch
    {
        rdg_render_pipeline pipeline;
        std::vector<rhi_parameter*> parameters;

        std::vector<draw_item> items;
    };

    std::vector<mesh> meshes;
    std::vector<batch> batches;

    rhi_parameter* light;
    rhi_parameter* camera;
};

class mesh;
class render_context
{
public:
    render_context();

    render_list get_render_list(const render_camera& camera) const;

    allocation allocate_constant(std::size_t size);
    void free_constant(allocation allocation);

    void update_constant(std::uint32_t offset, const void* data, std::size_t size);

    void update_resource();

private:
    std::vector<std::pair<const mesh*, rhi_parameter*>> m_meshes;

    rhi_ptr<rhi_parameter> m_light;

    allocator m_constant_allocator;
    rhi_ptr<rhi_buffer> m_constant_buffer;

    allocator m_staging_allocator;
    std::vector<rhi_ptr<rhi_buffer>> m_staging_buffers;

    struct update_buffer_command
    {
        rhi_buffer* src;
        rhi_buffer* dst;
        std::uint32_t src_offset;
        std::uint32_t dst_offset;
        std::size_t size;
    };
    std::vector<update_buffer_command> m_update_buffer_commands;

    friend class graphics_system;
};
} // namespace violet