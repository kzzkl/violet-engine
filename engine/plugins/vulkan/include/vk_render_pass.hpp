#pragma once

#include "vk_common.hpp"
#include <vector>

namespace violet::vk
{
class vk_context;
class vk_render_pass : public rhi_render_pass
{
public:
    vk_render_pass(const rhi_render_pass_desc& desc, vk_context* context);
    vk_render_pass(const vk_render_pass&) = delete;
    virtual ~vk_render_pass();

    vk_render_pass& operator=(const vk_render_pass&) = delete;

    void begin(
        VkCommandBuffer command_buffer,
        const rhi_attachment* attachments,
        std::size_t attachment_count,
        const rhi_texture_extent& render_area);

    void end(VkCommandBuffer command_buffer);

    VkRenderPass get_render_pass() const noexcept
    {
        return m_render_pass;
    }

    std::size_t get_render_target_count() const noexcept
    {
        return m_render_target_count;
    }

private:
    VkRenderPass m_render_pass;

    std::vector<rhi_attachment_type> m_attachment_layout;
    std::size_t m_render_target_count{0};

    vk_context* m_context;
};
} // namespace violet::vk