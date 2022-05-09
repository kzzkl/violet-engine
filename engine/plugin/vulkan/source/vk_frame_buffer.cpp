#include "vk_frame_buffer.hpp"
#include "vk_context.hpp"
#include "vk_pipeline.hpp"
#include "vk_renderer.hpp"
#include "vk_resource.hpp"

namespace ash::graphics::vk
{
vk_frame_buffer_layout::vk_frame_buffer_layout(attachment_desc* attachment, std::size_t count)
{
    for (std::size_t i = 0; i < count; ++i)
    {
        attachment_info info = {};
        info.type = attachment[i].type;
        info.description.format = to_vk_format(attachment[i].format);
        info.description.samples = to_vk_samples(attachment[i].samples);
        info.description.loadOp = to_vk_attachment_load_op(attachment[i].load_op);
        info.description.storeOp = to_vk_attachment_store_op(attachment[i].store_op);
        info.description.stencilLoadOp = to_vk_attachment_load_op(attachment[i].stencil_load_op);
        info.description.stencilStoreOp = to_vk_attachment_store_op(attachment[i].stencil_store_op);
        info.description.initialLayout = to_vk_image_layout(attachment[i].initial_state);
        info.description.finalLayout = to_vk_image_layout(attachment[i].final_state);

        m_attachments.push_back(info);
    }
}

vk_frame_buffer::vk_frame_buffer(vk_render_pass* render_pass, vk_image* render_target)
{
    auto device = vk_context::device();

    VkExtent2D extent = render_target->extent();
    std::vector<VkImageView> views;

    for (auto& attachment : render_pass->frame_buffer_layout())
    {
        VkClearValue clear_value = {};
        switch (attachment.type)
        {
        case attachment_type::COLOR: {
            auto color = std::make_unique<vk_render_target>(
                extent.width,
                extent.height,
                attachment.description.format,
                attachment.description.samples);
            views.push_back(color->view());
            m_attachments.push_back(std::move(color));
            clear_value.color = {0.0f, 0.0f, 0.0f, 1.0f};
            break;
        }
        case attachment_type::DEPTH: {
            auto depth = std::make_unique<vk_depth_stencil_buffer>(
                extent.width,
                extent.height,
                attachment.description.format,
                attachment.description.samples);
            views.push_back(depth->view());
            m_attachments.push_back(std::move(depth));
            clear_value.depthStencil = {1.0f, 0};
            break;
        }
        case attachment_type::RENDER_TARGET: {
            views.push_back(render_target->view());
            clear_value.color = {0.0f, 0.0f, 0.0f, 1.0f};
            break;
        }
        default:
            throw vk_exception("Invalid attachment type");
        }
        m_clear_values.push_back(clear_value);
    }

    VkFramebufferCreateInfo frame_buffer_info = {};
    frame_buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frame_buffer_info.renderPass = render_pass->render_pass();
    frame_buffer_info.attachmentCount = static_cast<std::uint32_t>(views.size());
    frame_buffer_info.pAttachments = views.data();
    frame_buffer_info.width = extent.width;
    frame_buffer_info.height = extent.height;
    frame_buffer_info.layers = 1;

    throw_if_failed(vkCreateFramebuffer(device, &frame_buffer_info, nullptr, &m_frame_buffer));
}

vk_frame_buffer::vk_frame_buffer(vk_frame_buffer&& other)
    : m_frame_buffer(other.m_frame_buffer),
      m_attachments(std::move(other.m_attachments)),
      m_clear_values(std::move(other.m_clear_values))
{
    other.m_frame_buffer = VK_NULL_HANDLE;
}

vk_frame_buffer::~vk_frame_buffer()
{
    if (m_frame_buffer != VK_NULL_HANDLE)
    {
        auto device = vk_context::device();
        vkDestroyFramebuffer(device, m_frame_buffer, nullptr);
    }
}

vk_frame_buffer& vk_frame_buffer::operator=(vk_frame_buffer&& other)
{
    m_frame_buffer = other.m_frame_buffer;
    m_attachments = std::move(other.m_attachments);
    m_clear_values = std::move(other.m_clear_values);

    other.m_frame_buffer = VK_NULL_HANDLE;

    return *this;
}

vk_frame_buffer* vk_frame_buffer_manager::get_or_create_frame_buffer(
    vk_render_pass* render_pass,
    vk_image* render_target)
{
    auto& result = m_frame_buffers[std::make_pair(render_pass, render_target->view())];
    if (result == nullptr)
        result = std::make_unique<vk_frame_buffer>(render_pass, render_target);

    return result.get();
}

void vk_frame_buffer_manager::notify_destroy(vk_image* render_target)
{
    for (auto iter = m_frame_buffers.begin(); iter != m_frame_buffers.end();)
    {
        if (iter->first.second == render_target->view())
            iter = m_frame_buffers.erase(iter);
        else
            ++iter;
    }
}
} // namespace ash::graphics::vk