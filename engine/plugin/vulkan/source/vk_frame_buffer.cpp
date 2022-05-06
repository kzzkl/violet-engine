#include "vk_frame_buffer.hpp"
#include "vk_context.hpp"
#include "vk_renderer.hpp"
#include "vk_resource.hpp"

namespace ash::graphics::vk
{
vk_frame_buffer::vk_frame_buffer(const attachment_set_desc& desc)
{
    auto device = vk_context::device();

    std::vector<VkImageView> views = {};
    for (std::size_t i = 0; i < desc.attachment_count; ++i)
    {
        vk_image* image = static_cast<vk_image*>(desc.attachments[i]);
        views.push_back(image->view());
    }

    VkFramebufferCreateInfo frame_buffer_info = {};
    frame_buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frame_buffer_info.renderPass = static_cast<vk_render_pass*>(desc.technique)->render_pass();
    frame_buffer_info.attachmentCount = static_cast<std::uint32_t>(views.size());
    frame_buffer_info.pAttachments = views.data();
    frame_buffer_info.width = desc.width;
    frame_buffer_info.height = desc.height;
    frame_buffer_info.layers = 1;

    throw_if_failed(vkCreateFramebuffer(device, &frame_buffer_info, nullptr, &m_frame_buffer));
}
} // namespace ash::graphics::vk