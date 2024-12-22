#pragma once

#include "vk_sync.hpp"
#include <memory>
#include <vector>

namespace violet::vk
{
class vk_command : public rhi_command
{
public:
    vk_command(VkCommandBuffer command_buffer, vk_context* context) noexcept;
    virtual ~vk_command();

    VkCommandBuffer get_command_buffer() const noexcept
    {
        return m_command_buffer;
    }

    void begin_render_pass(rhi_render_pass* render_pass, rhi_framebuffer* framebuffer) override;
    void end_render_pass() override;
    void next_subpass() override;

    void set_pipeline(rhi_render_pipeline* render_pipeline) override;
    void set_pipeline(rhi_compute_pipeline* compute_pipeline) override;
    void set_parameter(std::size_t index, rhi_parameter* parameter) override;

    void set_viewport(const rhi_viewport& viewport) override;
    void set_scissor(const rhi_scissor_rect* rects, std::size_t size) override;

    void set_vertex_buffers(rhi_buffer* const* vertex_buffers, std::size_t vertex_buffer_count)
        override;
    void set_index_buffer(rhi_buffer* index_buffer) override;

    void draw(std::size_t vertex_offset, std::size_t vertex_count) override;
    void draw_indexed(std::size_t index_offset, std::size_t index_count, std::size_t vertex_base)
        override;
    void draw_indexed_indirect(
        rhi_buffer* command_buffer,
        std::size_t command_buffer_offset,
        rhi_buffer* count_buffer,
        std::size_t count_buffer_offset,
        std::size_t max_draw_count) override;

    void dispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z) override;

    void set_pipeline_barrier(
        rhi_pipeline_stage_flags src_stages,
        rhi_pipeline_stage_flags dst_stages,
        const rhi_buffer_barrier* buffer_barriers,
        std::size_t buffer_barrier_count,
        const rhi_texture_barrier* texture_barriers,
        std::size_t texture_barrier_count) override;

    void copy_texture(
        rhi_texture* src,
        const rhi_texture_region& src_region,
        rhi_texture* dst,
        const rhi_texture_region& dst_region) override;

    void blit_texture(
        rhi_texture* src,
        const rhi_texture_region& src_region,
        rhi_texture* dst,
        const rhi_texture_region& dst_region) override;

    void fill_buffer(rhi_buffer* buffer, const rhi_buffer_region& region, std::uint32_t value)
        override;

    void copy_buffer(
        rhi_buffer* src,
        const rhi_buffer_region& src_region,
        rhi_buffer* dst,
        const rhi_buffer_region& dst_region) override;

    void copy_buffer_to_texture(
        rhi_buffer* buffer,
        const rhi_buffer_region& buffer_region,
        rhi_texture* texture,
        const rhi_texture_region& texture_region) override;

    void signal(rhi_fence* fence, std::uint64_t value) override;
    void wait(rhi_fence* fence, std::uint64_t value, rhi_pipeline_stage_flags stages) override;

#ifndef NDEBUG
    void begin_label(const char* label) const override
    {
        VkDebugUtilsLabelEXT info = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
            .pNext = nullptr,
            .pLabelName = label,
            .color = {1.0f, 1.0f, 1.0f, 1.0f}};

        vkCmdBeginDebugUtilsLabelEXT(m_command_buffer, &info);
    }

    void end_label() const override
    {
        vkCmdEndDebugUtilsLabelEXT(m_command_buffer);
    }
#endif

    void reset();

    const std::vector<VkSemaphore>& get_signal_fences() const noexcept
    {
        return m_signal_fences;
    }

    const std::vector<std::uint64_t>& get_signal_values() const noexcept
    {
        return m_signal_values;
    }

    const std::vector<VkSemaphore>& get_wait_fences() const noexcept
    {
        return m_wait_fences;
    }

    const std::vector<std::uint64_t>& get_wait_values() const noexcept
    {
        return m_wait_values;
    }

    const std::vector<VkPipelineStageFlags>& get_wait_stages() const noexcept
    {
        return m_wait_stages;
    }

private:
    VkCommandBuffer m_command_buffer;

    VkRenderPass m_current_render_pass;
    VkPipelineLayout m_current_pipeline_layout;
    VkPipelineBindPoint m_current_bind_point;

    std::vector<VkSemaphore> m_signal_fences;
    std::vector<std::uint64_t> m_signal_values;

    std::vector<VkSemaphore> m_wait_fences;
    std::vector<std::uint64_t> m_wait_values;
    std::vector<VkPipelineStageFlags> m_wait_stages;

    vk_context* m_context;
};

class vk_graphics_queue
{
public:
    vk_graphics_queue(std::uint32_t queue_family_index, vk_context* context);
    ~vk_graphics_queue();

    vk_command* allocate_command();

    void execute(rhi_command* command);
    void execute_sync(rhi_command* command);

    void begin_frame();

    VkQueue get_queue() const noexcept
    {
        return m_queue;
    }
    std::uint32_t get_family_index() const noexcept
    {
        return m_family_index;
    }

private:
    VkQueue m_queue;
    std::uint32_t m_family_index;

    VkCommandPool m_command_pool;
    std::vector<std::vector<vk_command*>> m_active_commands;
    std::vector<vk_command*> m_free_commands;
    std::vector<std::unique_ptr<vk_command>> m_commands;

    std::unique_ptr<vk_fence> m_fence;
    std::uint64_t m_fence_value{0};

    vk_context* m_context;
};

class vk_present_queue
{
public:
    vk_present_queue(std::uint32_t queue_family_index, vk_context* context);
    ~vk_present_queue();

    void present(VkSwapchainKHR swapchain, std::uint32_t image_index, VkSemaphore wait_semaphore);

    VkQueue get_queue() const noexcept
    {
        return m_queue;
    }
    std::uint32_t get_family_index() const noexcept
    {
        return m_family_index;
    }

private:
    VkQueue m_queue;
    std::uint32_t m_family_index;
};
} // namespace violet::vk