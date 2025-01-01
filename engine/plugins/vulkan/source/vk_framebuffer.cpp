#include "vk_framebuffer.hpp"
#include "vk_render_pass.hpp"
#include "vk_resource.hpp"
#include <span>
#include <vector>

namespace violet::vk
{
vk_framebuffer::vk_framebuffer(const rhi_framebuffer_desc& desc, vk_context* context)
    : m_context(context)
{
    std::vector<VkImageView> image_views;

    std::uint32_t attachment_count = 0;
    for (auto* attachment : std::span(desc.attachments))
    {
        if (attachment == nullptr)
        {
            break;
        }

        const vk_texture* image = static_cast<const vk_texture*>(attachment);
        image_views.push_back(image->get_image_view());
        m_clear_values.push_back(image->get_clear_value());

        ++attachment_count;
    }

    m_extent = desc.attachments[0]->get_extent();

    VkFramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = static_cast<vk_render_pass*>(desc.render_pass)->get_render_pass();
    framebuffer_info.pAttachments = image_views.data();
    framebuffer_info.attachmentCount = attachment_count;
    framebuffer_info.width = m_extent.width;
    framebuffer_info.height = m_extent.height;
    framebuffer_info.layers = 1;

    vk_check(
        vkCreateFramebuffer(context->get_device(), &framebuffer_info, nullptr, &m_framebuffer));
}

vk_framebuffer::~vk_framebuffer()
{
    if (m_framebuffer)
    {
        vkDestroyFramebuffer(m_context->get_device(), m_framebuffer, nullptr);
    }
}
} // namespace violet::vk