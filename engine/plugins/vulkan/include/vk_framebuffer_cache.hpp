#pragma once

#include "vk_common.hpp"
#include <unordered_map>

namespace violet::vk
{
class vk_framebuffer_cache
{
public:
    vk_framebuffer_cache(VkDevice device);

    VkFramebuffer get_framebuffer(
        VkRenderPass render_pass,
        VkImageView* attachments,
        std::uint32_t attachment_count,
        std::uint32_t width,
        std::uint32_t height,
        VkClearValue* clear_value = nullptr);

    void on_image_destroy(VkImageView image_view);

private:
    using key_type = std::size_t;
    struct value_type
    {
        VkFramebuffer framebuffer;
        VkRenderPass render_pass;
        std::vector<VkImageView> attachments;
    };

    std::unordered_map<key_type, value_type> m_framebuffers;

    VkDevice m_device;
};
} // namespace violet::vk