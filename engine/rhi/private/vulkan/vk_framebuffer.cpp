#include "vk_framebuffer.hpp"
#include "vk_context.hpp"
#include "vk_render_pass.hpp"
#include "vk_resource.hpp"
#include <vector>

namespace violet::vk
{
vk_framebuffer_manager::vk_framebuffer_manager(vk_context* context)
    : m_context(context)
{
}

vk_framebuffer_manager::~vk_framebuffer_manager()
{
    for (auto& [key, framebuffer] : m_framebuffers)
    {
        vkDestroyFramebuffer(m_context->get_device(), framebuffer, nullptr);
    }
}

VkFramebuffer vk_framebuffer_manager::allocate_framebuffer(
    VkRenderPass render_pass,
    const std::vector<VkImageView>& image_views,
    const VkExtent2D& extent)
{
    framebuffer_key key = {};
    key.render_pass = render_pass;
    key.image_views = image_views;

    auto iter = m_framebuffers.find(key);
    if (iter != m_framebuffers.end())
    {
        return iter->second;
    }

    VkFramebufferCreateInfo framebuffer_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = static_cast<std::uint32_t>(image_views.size()),
        .pAttachments = image_views.data(),
        .width = extent.width,
        .height = extent.height,
        .layers = 1,
    };

    vk_check(vkCreateFramebuffer(
        m_context->get_device(),
        &framebuffer_info,
        nullptr,
        &m_framebuffers[key]));

    return m_framebuffers[key];
}

void vk_framebuffer_manager::notify_texture_deleted(VkImageView image_view)
{
    for (auto iter = m_framebuffers.begin(); iter != m_framebuffers.end();)
    {
        bool erase = false;

        for (VkImageView view : iter->first.image_views)
        {
            if (view == image_view)
            {
                erase = true;
                break;
            }
        }

        if (erase)
        {
            vkDestroyFramebuffer(m_context->get_device(), iter->second, nullptr);
            iter = m_framebuffers.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

void vk_framebuffer_manager::notify_render_pass_deleted(VkRenderPass render_pass)
{
    for (auto iter = m_framebuffers.begin(); iter != m_framebuffers.end();)
    {
        if (iter->first.render_pass == render_pass)
        {
            vkDestroyFramebuffer(m_context->get_device(), iter->second, nullptr);
            iter = m_framebuffers.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}
} // namespace violet::vk