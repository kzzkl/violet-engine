#pragma once

#include "graphics/render_graph/rdg_allocator.hpp"
#include "graphics/render_graph/rdg_pipeline.hpp"
#include "graphics/render_scene.hpp"

namespace violet
{
class rdg_command
{
public:
    rdg_command(rhi_command* command, rdg_allocator* allocator);

    void set_pipeline(const rdg_render_pipeline& pipeline);
    void set_pipeline(const rdg_compute_pipeline& pipeline);

    void set_parameter(std::size_t index, rhi_parameter* parameter)
    {
        m_command->set_parameter(index, parameter);
    }

    void set_viewport(const rhi_viewport& viewport)
    {
        m_command->set_viewport(viewport);
    }

    void set_scissor(std::span<const rhi_scissor_rect> rects)
    {
        m_command->set_scissor(rects.data(), rects.size());
    }

    void set_vertex_buffers(std::span<rhi_buffer* const> vertex_buffers)
    {
        m_command->set_vertex_buffers(vertex_buffers.data(), vertex_buffers.size());
    }

    void set_index_buffer(rhi_buffer* index_buffer)
    {
        m_command->set_index_buffer(index_buffer);
    }

    void draw(std::size_t vertex_offset, std::size_t vertex_count)
    {
        m_command->draw(vertex_offset, vertex_count);
    }

    void draw_indexed(std::size_t index_offset, std::size_t index_count, std::size_t vertex_base)
    {
        m_command->draw_indexed(index_offset, index_count, vertex_base);
    }

    void draw_instances(
        const render_scene& scene,
        const render_camera& camera,
        rhi_buffer* command_buffer,
        rhi_buffer* count_buffer,
        material_type type);

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

    void set_pipeline_barrier(
        rhi_pipeline_stage_flags src_stage,
        rhi_pipeline_stage_flags dst_stage,
        std::span<rhi_buffer_barrier> buffer_barriers,
        std::span<rhi_texture_barrier> texture_barriers)
    {
        m_command->set_pipeline_barrier(
            src_stage,
            dst_stage,
            buffer_barriers.data(),
            buffer_barriers.size(),
            texture_barriers.data(),
            texture_barriers.size());
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
        const rhi_texture_region& dst_region)
    {
        m_command->blit_texture(src, src_region, dst, dst_region);
    }

    void fill_buffer(rhi_buffer* buffer, const rhi_buffer_region& region, std::uint32_t value)
    {
        m_command->fill_buffer(buffer, region, value);
    }

private:
    rhi_render_pass* m_render_pass{nullptr};
    std::uint32_t m_subpass_index{0};

    rhi_command* m_command;
    rdg_allocator* m_allocator;

    friend class render_graph;
};
} // namespace violet