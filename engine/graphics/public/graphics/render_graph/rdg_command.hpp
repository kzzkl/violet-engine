#pragma once

#include "graphics/render_context.hpp"
#include "graphics/render_graph/rdg_allocator.hpp"
#include "graphics/render_graph/rdg_pipeline.hpp"

namespace violet
{
class rdg_command
{
public:
    rdg_command(rhi_command* command, rdg_allocator* allocator);

    void begin_render_pass(rhi_render_pass* render_pass, rhi_framebuffer* framebuffer);
    void end_render_pass();
    void next_subpass();

    void set_pipeline(const rdg_render_pipeline& pipeline);
    void set_pipeline(rhi_compute_pipeline* compute_pipeline)
    {
        m_command->set_pipeline(compute_pipeline);
    }
    void set_parameter(std::size_t index, rhi_parameter* parameter)
    {
        m_command->set_parameter(index, parameter);
    }

    void set_viewport(const rhi_viewport& viewport) { m_command->set_viewport(viewport); }
    void set_scissor(const std::vector<rhi_scissor_rect>& rects)
    {
        m_command->set_scissor(rects.data(), rects.size());
    }

    void set_vertex_buffers(const std::vector<rhi_buffer*>& vertex_buffers)
    {
        m_command->set_vertex_buffers(vertex_buffers.data(), vertex_buffers.size());
    }
    void set_index_buffer(rhi_buffer* index_buffer) { m_command->set_index_buffer(index_buffer); }

    void draw(std::size_t vertex_start, std::size_t vertex_count)
    {
        m_command->draw(vertex_start, vertex_count);
    }
    void draw_indexed(std::size_t index_start, std::size_t index_count, std::size_t vertex_base)
    {
        m_command->draw_indexed(index_start, index_count, vertex_base);
    }

    void draw_render_list(const render_list& render_list);

    void dispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z) {}

    void set_pipeline_barrier(
        rhi_pipeline_stage_flags src_stage,
        rhi_pipeline_stage_flags dst_stage,
        const std::vector<rhi_buffer_barrier>& buffer_barriers,
        const std::vector<rhi_texture_barrier>& texture_barriers)
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
        const rhi_resource_region& src_region,
        rhi_texture* dst,
        const rhi_resource_region& dst_region)
    {
        m_command->copy_texture(src, src_region, dst, dst_region);
    }

private:
    rhi_render_pass* m_render_pass{nullptr};
    std::uint32_t m_subpass_index{0};

    rhi_command* m_command;
    rdg_allocator* m_allocator;
};
} // namespace violet