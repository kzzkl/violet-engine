#pragma once

#include "vk_common.hpp"
#include <memory>
#include <vector>

namespace violet::vk
{
class vk_rhi;
class vk_render_pass;
class vk_command : public render_command_interface
{
public:
    vk_command(VkCommandBuffer command_buffer, vk_rhi* rhi) noexcept;
    virtual ~vk_command();

    VkCommandBuffer get_command_buffer() const noexcept { return m_command_buffer; }

public:
    virtual void begin(
        render_pass_interface* render_pass,
        resource_interface* const* attachments,
        std::size_t attachment_count) override;
    virtual void end() override;
    virtual void next() override;

    virtual void set_pipeline(render_pipeline_interface* render_pipeline) override;
    virtual void set_parameter(std::size_t index, pipeline_parameter_interface* parameter) override;

    virtual void set_viewport(
        float x,
        float y,
        float width,
        float height,
        float min_depth,
        float max_depth) override;
    virtual void set_scissor(const scissor_extent* extents, std::size_t size) override;

    virtual void set_input_assembly_state(
        resource_interface* const* vertex_buffers,
        std::size_t vertex_buffer_count,
        resource_interface* index_buffer,
        primitive_topology primitive_topology) override;

    virtual void draw(std::size_t vertex_start, std::size_t vertex_end) override;
    virtual void draw_indexed(
        std::size_t index_start,
        std::size_t index_end,
        std::size_t vertex_base) override;

    virtual void clear_render_target(resource_interface* render_target, const float4& color)
        override;
    virtual void clear_depth_stencil(
        resource_interface* depth_stencil,
        bool clear_depth,
        float depth,
        bool clear_stencil,
        std::uint8_t stencil) override;

private:
    VkCommandBuffer m_command_buffer;

    vk_render_pass* m_current_render_pass;
    vk_rhi* m_rhi;
};

class vk_command_queue
{
public:
    vk_command_queue(std::uint32_t queue_family_index, vk_rhi* rhi);
    ~vk_command_queue();

    vk_command* allocate_command();

private:
    void allocate_command_buffer(std::uint32_t count);

    VkCommandPool m_command_pool;

    std::vector<std::vector<vk_command*>> m_active_commands;
    std::vector<vk_command*> m_free_commands;
    std::vector<std::unique_ptr<vk_command>> m_commands;

    vk_rhi* m_rhi;
};
} // namespace violet::vk