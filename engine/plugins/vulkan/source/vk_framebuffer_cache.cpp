#include "vk_framebuffer_cache.hpp"

namespace violet::vk
{
vk_framebuffer_cache::vk_framebuffer_cache(VkDevice device) : m_device(device)
{
}

VkFramebuffer vk_framebuffer_cache::get_framebuffer(
    VkRenderPass render_pass,
    VkImageView* attachments,
    std::uint32_t attachment_count,
    std::uint32_t width,
    std::uint32_t height,
    VkClearValue* clear_value)
{
    if (clear_value)
    {
        clear_value->color = {0.0f, 0.0f, 0.0f, 1.0f};
        clear_value->depthStencil = {1.0f, 0};
    }

    std::size_t key = std::hash<void*>()(render_pass);
    for (std::uint32_t i = 0; i < attachment_count; ++i)
        key ^= std::hash<void*>()(attachments[i]) + 0x9e3779b9 + (key << 6) + (key >> 2);

    auto iter = m_framebuffers.find(key);
    if (iter != m_framebuffers.end())
        return iter->second.framebuffer;

    value_type value = {};
    for (std::uint32_t i = 0; i < attachment_count; ++i)
        value.attachments.push_back(attachments[i]);

    VkFramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass;
    framebuffer_info.pAttachments = attachments;
    framebuffer_info.attachmentCount = attachment_count;
    framebuffer_info.width = width;
    framebuffer_info.height = height;
    framebuffer_info.layers = 1;

    throw_if_failed(vkCreateFramebuffer(m_device, &framebuffer_info, nullptr, &value.framebuffer));
    m_framebuffers.insert(std::make_pair(key, value));
    return value.framebuffer;
}

void vk_framebuffer_cache::on_destroy_image(VkImageView image_view)
{
    for (auto iter = m_framebuffers.begin(); iter != m_framebuffers.end();)
    {
        bool found = false;
        for (VkImageView view : iter->second.attachments)
        {
            if (view == image_view)
            {
                found = true;
                break;
            }
        }

        if (found)
        {
            vkDestroyFramebuffer(m_device, iter->second.framebuffer, nullptr);
            iter = m_framebuffers.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}
} // namespace violet::vk