#include "vk_framebuffer.hpp"
#include "vk_render_pass.hpp"
#include "vk_resource.hpp"
#include <vector>

namespace violet::vk
{
vk_framebuffer::vk_framebuffer(const rhi_framebuffer_desc& desc, vk_context* context)
    : m_context(context)
{
    std::vector<VkImageView> image_views(desc.attachment_count);
    for (std::size_t i = 0; i < desc.attachment_count; ++i)
    {
        const vk_image* image = static_cast<const vk_image*>(desc.attachments[i]);
        image_views[i] = image->get_image_view();
        m_clear_values.push_back(image->get_clear_value());
    }

    m_extent = desc.attachments[0]->get_extent();

    VkFramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = static_cast<vk_render_pass*>(desc.render_pass)->get_render_pass();
    framebuffer_info.pAttachments = image_views.data();
    framebuffer_info.attachmentCount = static_cast<std::uint32_t>(image_views.size());
    framebuffer_info.width = m_extent.width;
    framebuffer_info.height = m_extent.height;
    framebuffer_info.layers = 1;

    throw_if_failed(
        vkCreateFramebuffer(context->get_device(), &framebuffer_info, nullptr, &m_framebuffer));
}

vk_framebuffer::~vk_framebuffer()
{
    if (m_framebuffer)
        vkDestroyFramebuffer(m_context->get_device(), m_framebuffer, nullptr);
}
} // namespace violet::vk