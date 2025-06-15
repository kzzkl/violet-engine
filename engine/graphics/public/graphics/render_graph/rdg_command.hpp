#pragma once

#include "graphics/material.hpp"
#include "graphics/render_graph/rdg_allocator.hpp"
#include "graphics/render_graph/rdg_pipeline.hpp"
#include <array>

namespace violet
{
class render_scene;
class render_camera;

enum rdg_parameter_type
{
    RDG_PARAMETER_BINDLESS,
    RDG_PARAMETER_SCENE,
    RDG_PARAMETER_CAMERA,
};

class rdg_command
{
public:
    rdg_command(
        rhi_command* command,
        rdg_allocator* allocator,
        const render_scene* scene,
        const render_camera* camera);

    void set_pipeline(const rdg_raster_pipeline& pipeline);
    void set_pipeline(const rdg_compute_pipeline& pipeline);

    void set_parameter(std::uint32_t index, rdg_parameter_type type)
    {
        m_command->set_parameter(index, m_built_in_parameters[type]);
    }

    void set_parameter(std::uint32_t index, rhi_parameter* parameter)
    {
        m_command->set_parameter(index, parameter);
    }

    template <typename T>
    void set_constant(const T& constant)
    {
        m_command->set_constant(&constant, sizeof(T));
    }

    void set_viewport();
    void set_viewport(const rhi_viewport& viewport)
    {
        m_command->set_viewport(viewport);
    }

    void set_scissor();
    void set_scissor(std::span<const rhi_scissor_rect> rects)
    {
        m_command->set_scissor(rects.data(), static_cast<std::uint32_t>(rects.size()));
    }

    void set_vertex_buffers(std::span<rhi_buffer* const> vertex_buffers)
    {
        m_command->set_vertex_buffers(
            vertex_buffers.data(),
            static_cast<std::uint32_t>(vertex_buffers.size()));
    }

    void set_index_buffer(rhi_buffer* index_buffer, std::size_t index_size)
    {
        m_command->set_index_buffer(index_buffer, index_size);
    }

    void draw(
        std::uint32_t vertex_offset,
        std::uint32_t vertex_count,
        std::uint32_t instance_offset = 0,
        std::uint32_t instance_count = 1)
    {
        m_command->draw(vertex_offset, vertex_count, instance_offset, instance_count);
    }

    void draw_indexed(
        std::uint32_t index_offset,
        std::uint32_t index_count,
        std::uint32_t vertex_offset,
        std::uint32_t instance_offset = 0,
        std::uint32_t instance_count = 1)
    {
        m_command->draw_indexed(
            index_offset,
            index_count,
            vertex_offset,
            instance_offset,
            instance_count);
    }

    void draw_instances(rhi_buffer* command_buffer, rhi_buffer* count_buffer, material_type type);
    void draw_instances(
        rhi_buffer* command_buffer,
        rhi_buffer* count_buffer,
        material_type type,
        const rdg_raster_pipeline& pipeline);

    void draw_fullscreen()
    {
        draw(0, 3);
    }

    void dispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z)
    {
        m_command->dispatch(x, y, z);
    }

    void dispatch_1d(std::uint32_t x, std::uint32_t thread_group_x = 64)
    {
        dispatch((x + thread_group_x - 1) / thread_group_x, 1, 1);
    }

    void dispatch_2d(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t thread_group_x = 8,
        std::uint32_t thread_group_y = 8)
    {
        dispatch(
            (x + thread_group_x - 1) / thread_group_x,
            (y + thread_group_y - 1) / thread_group_y,
            1);
    }

    void dispatch_3d(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t z,
        std::uint32_t thread_group_x = 4,
        std::uint32_t thread_group_y = 4,
        std::uint32_t thread_group_z = 4)
    {
        dispatch(
            (x + thread_group_x - 1) / thread_group_x,
            (y + thread_group_y - 1) / thread_group_y,
            (z + thread_group_z - 1) / thread_group_z);
    }

    void dispatch_indirect(rhi_buffer* command_buffer, std::uint32_t command_buffer_offset)
    {
        m_command->dispatch_indirect(command_buffer, command_buffer_offset);
    }

    void copy_texture(
        rhi_texture* src,
        const rhi_texture_region& src_region,
        rhi_texture* dst,
        const rhi_texture_region& dst_region)
    {
        m_command->copy_texture(src, src_region, dst, dst_region);
    }

    void blit_texture(
        rhi_texture* src,
        const rhi_texture_region& src_region,
        rhi_texture* dst,
        const rhi_texture_region& dst_region,
        rhi_filter filter)
    {
        m_command->blit_texture(src, src_region, dst, dst_region, filter);
    }

    void fill_buffer(rhi_buffer* buffer, const rhi_buffer_region& region, std::uint32_t value)
    {
        m_command->fill_buffer(buffer, region, value);
    }

    rhi_command* get_rhi() const noexcept
    {
        return m_command;
    }

private:
    rhi_render_pass* m_render_pass{nullptr};
    std::uint32_t m_subpass_index{0};

    rhi_command* m_command;
    rdg_allocator* m_allocator;

    const render_scene* m_scene;
    const render_camera* m_camera;

    std::array<rhi_parameter*, 3> m_built_in_parameters;

    friend class render_graph;
};
} // namespace violet